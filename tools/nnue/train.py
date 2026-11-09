"""Trains an NNUE net for Regicide on DataRecord files (datagen output) and exports it as RGNN.

  python tools/nnue/train.py --data a.bin --data b.bin --out runs/net1 [--epochs 40] [--lambda 0.7]
                             [--arch v1|v2] [--ft N] [--l1 N] [--l2 N] [--buckets N]

Target (as tools/train.cpp): lambda * game result + (1 - lambda) * sigmoid(search score / K), with K fitted to
the data unless given; loss is cross-entropy against sigmoid(net score / K). Each epoch writes a checkpoint
(model + optimiser, for --resume) and an exported net, runs/net1/net_eNNN.nnue, which the engine loads with
"set evalfile". See README.md.
"""

import argparse
import json
import math
import os
import sys
import time

import numpy as np
import torch
import torch.nn.functional as F

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import nnue  # noqa: E402


def parse_args(argv=None):
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--data", action="append", required=True, help="DataRecord file (repeatable)")
    p.add_argument("--out", required=True, help="output directory for checkpoints and nets")
    p.add_argument("--arch", choices=sorted(nnue.DEFAULTS), default="v1",
                   help="v1: L1 sees the side to move's FT output, output buckets only; v2: L1 sees both points "
                        "of view (2 x ft inputs), L1/L2/output bucketed")
    p.add_argument("--ft", type=int, help="FT width per point of view (default: v1 %d, v2 %d)" %
                   (nnue.V1.ft, nnue.V2.ft))
    p.add_argument("--l1", type=int, help="L1 width, 16/32/64 (default 32)")
    p.add_argument("--l2", type=int, help="L2 width, 16/32/64 (default 32)")
    p.add_argument("--buckets", type=int, help="piece-count buckets, 1..24 (default 8)")
    p.add_argument("--epochs", type=int, default=40)
    p.add_argument("--epoch-size", type=int, default=0, help="positions per epoch (default: the training set)")
    p.add_argument("--batch", type=int, default=16384)
    p.add_argument("--lr", type=float, default=1e-3)
    p.add_argument("--lr-final", type=float, default=1e-5, help="cosine schedule ends here")
    p.add_argument("--warmup", type=int, default=200, help="linear warmup steps")
    p.add_argument("--wd", type=float, default=1e-2, help="AdamW decoupled weight decay")
    p.add_argument("--lambda", dest="lam", type=float, default=0.7, help="weight of the game result")
    p.add_argument("--k", type=float, default=0, help="score scale K (0 = fit to the data)")
    p.add_argument("--valid-frac", type=float, default=0.02, help="tail of each file held out")
    p.add_argument("--min-pieces", type=int, default=0,
                   help="leave out records with fewer pieces (9: what the 8-piece database never lets the net see)")
    p.add_argument("--valid-max", type=int, default=1_000_000, help="validation positions used")
    p.add_argument("--qat-epochs", type=int, default=2,
                   help="final epochs trained through the simulated integer forward pass")
    p.add_argument("--out-scale", type=int, default=nnue.OUT_SCALE, help="centi-men per unit of net output")
    p.add_argument("--device", default="auto", help="cuda, cpu, ... (auto: cuda if available)")
    p.add_argument("--threads", type=int, default=0, help="torch CPU threads (0 = default)")
    p.add_argument("--loader-threads", type=int, default=2)
    p.add_argument("--compile", action="store_true", help="torch.compile the model (Linux/WSL)")
    p.add_argument("--resume", help="checkpoint to continue from (its model, optimiser, K and epoch)")
    p.add_argument("--seed", type=int, default=1)
    p.add_argument("--max-minutes", type=float, default=0, help="stop (and export) after this long")
    return p.parse_args(argv)


def lr_at(step, total, args):
    if step < args.warmup:
        return args.lr * (step + 1) / args.warmup
    t = min(1.0, (step - args.warmup) / max(1, total - args.warmup))
    return args.lr_final + 0.5 * (args.lr - args.lr_final) * (1 + math.cos(math.pi * t))


def batch_loss(model, arch, t, K, lam, quant):
    x, xo, b = nnue.inputs(arch, t["black"], t["white"], t["kings"], t["stm"])
    pred = model(x, xo, b, quant=quant)
    target = lam * t["result"] + (1 - lam) * torch.sigmoid(t["score"].float() / K)
    # A proven score (stm flag, src/datafile.hpp) is the exact result: it is the target whatever lambda is.
    target = torch.where(t["proven"], (t["score"] > 0).float(), target)
    return F.binary_cross_entropy_with_logits(pred / K, target), pred


@torch.no_grad()
def validate(model, valid, K, lam, device, quant):
    total, n = 0.0, 0
    for i in range(0, len(valid), 65536):
        t = nnue.to_tensors(valid[i:i + 65536], device)
        loss, _ = batch_loss(model, model.arch, t, K, lam, quant)
        total += loss.item() * len(valid[i:i + 65536])
        n += len(valid[i:i + 65536])
    return total / max(1, n)


def main(argv=None):
    args = parse_args(argv)
    os.makedirs(args.out, exist_ok=True)
    device = torch.device("cuda" if args.device == "auto" and torch.cuda.is_available() else
                          "cpu" if args.device == "auto" else args.device)
    if args.threads:
        torch.set_num_threads(args.threads)
    torch.set_float32_matmul_precision("highest")  # the integer simulation needs exact float32 sums
    torch.manual_seed(args.seed)
    rng = np.random.default_rng(args.seed)

    data = nnue.Data(args.data, args.valid_frac, args.min_pieces)
    if data.n_train == 0:
        sys.exit("no training data")
    valid = data.validation(args.valid_max, rng)

    arch = nnue.make_arch(args.arch, args.ft, args.l1, args.l2, args.buckets)
    start_epoch, step, K = 1, 0, args.k
    ck = torch.load(args.resume, map_location=device) if args.resume else None
    if ck and nnue.arch_of_state(ck["model"]) != arch:
        sys.exit("--resume: the checkpoint is %s, the arguments ask for %s" % (nnue.arch_of_state(ck["model"]), arch))
    model = nnue.Net(arch, args.out_scale).to(device)
    opt = torch.optim.AdamW(model.parameters(), lr=args.lr, weight_decay=args.wd)
    if ck:
        model.load_state_dict(ck["model"])
        opt.load_state_dict(ck["optimizer"])
        start_epoch, step, K = ck["epoch"] + 1, ck["step"], ck["K"]
        print("resumed from %s at epoch %d" % (args.resume, ck["epoch"]))
    if K <= 0:
        K = nnue.fit_k(data.sample(rng, min(data.n_train, 2_000_000)))
    model.clip_()
    fwd = torch.compile(model) if args.compile else model

    epoch_size = args.epoch_size or data.n_train
    steps_per_epoch = max(1, epoch_size // args.batch)
    total_steps = steps_per_epoch * args.epochs
    print("net %s  train %d  valid %d  K %.1f  lambda %.2f  min pieces %d  device %s  %d steps/epoch" %
          (arch, data.n_train, len(valid), K, args.lam, args.min_pieces, device, steps_per_epoch), flush=True)
    with open(os.path.join(args.out, "config.json"), "w") as f:
        json.dump(dict(vars(args), K=K, net=dict(arch._asdict())), f, indent=1)

    loader = nnue.Prefetcher(data, args.batch, args.seed * 1000 + start_epoch, workers=args.loader_threads)
    t0 = time.time()
    try:
        for epoch in range(start_epoch, args.epochs + 1):
            quant = epoch > args.epochs - args.qat_epochs
            model.train()
            train_loss, te = 0.0, time.time()
            for _ in range(steps_per_epoch):
                for g in opt.param_groups:
                    g["lr"] = lr_at(step, total_steps, args)
                t = nnue.to_tensors(loader.next(), device)
                loss, _ = batch_loss(fwd, arch, t, K, args.lam, quant)
                opt.zero_grad(set_to_none=True)
                loss.backward()
                opt.step()
                model.clip_()
                train_loss += loss.item()
                step += 1
            model.eval()
            vf = validate(model, valid, K, args.lam, device, False)
            vq = validate(model, valid, K, args.lam, device, True)
            print("epoch %3d  lr %.2e  train %.6f  valid %.6f  valid(int) %.6f%s  %.0f pos/s" %
                  (epoch, opt.param_groups[0]["lr"], train_loss / steps_per_epoch, vf, vq,
                   "  [qat]" if quant else "", steps_per_epoch * args.batch / (time.time() - te)), flush=True)
            ck = {"model": model.state_dict(), "optimizer": opt.state_dict(), "epoch": epoch, "step": step,
                  "K": K, "args": vars(args)}
            torch.save(ck, os.path.join(args.out, "last.pt"))
            torch.save(ck, os.path.join(args.out, "ckpt_e%03d.pt" % epoch))
            net = os.path.join(args.out, "net_e%03d.nnue" % epoch)
            nnue.write_rgnn(net, nnue.quantise(model))
            if args.max_minutes and time.time() - t0 > 60 * args.max_minutes:
                print("time limit reached")
                break
    finally:
        loader.close()
    print("wrote %s" % net)


if __name__ == "__main__":
    main()
