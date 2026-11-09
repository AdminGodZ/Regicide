"""Checks that an exported net evaluates identically in Python and in the engine.

  python tools/nnue/check_export.py --engine build/regicide --data positions.bin [--checkpoint last.pt] [--n 1000]
                                   [--arch v1|v2 --ft N --l1 N --l2 N --buckets N]   (random net)

Exports the checkpoint (or a random net of the given architecture, with weights spread over the quantisation
ranges) to RGNN, evaluates
--n positions from the data file with the Python integer forward pass (nnue.forward_int) and with
"regicide evaldata", and fails if any score differs by more than --tolerance centi-men. Also reports how far
the integer net is from the float model it was quantised from.
"""

import argparse
import os
import subprocess
import sys
import tempfile

import numpy as np
import torch

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import nnue  # noqa: E402


def random_model(arch, seed):
    torch.manual_seed(seed)
    m = nnue.Net(arch)
    # L1 sums grow with its input count; these ranges were set for 1024 inputs.
    l1_scale = (1024 / arch.l1_inputs) ** 0.5
    with torch.no_grad():
        m.ft.weight.uniform_(-1.2, 1.2)
        m.ft.bias.uniform_(-0.5, 1.0)
        m.l1.weight.uniform_(-0.4 * l1_scale, 0.4 * l1_scale)
        m.l1.bias.uniform_(-0.5, 0.5)
        m.l2.weight.uniform_(-0.6, 0.6)
        m.l2.bias.uniform_(-0.3, 0.6)
        m.out.weight.uniform_(-3, 3)
        m.out.bias.uniform_(-1, 1)
    m.clip_()
    return m


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--engine", required=True)
    p.add_argument("--data", required=True)
    p.add_argument("--checkpoint")
    p.add_argument("--net", help="also keep the exported net here")
    p.add_argument("--arch", choices=sorted(nnue.DEFAULTS), default="v1", help="random net: architecture")
    p.add_argument("--ft", type=int)
    p.add_argument("--l1", type=int)
    p.add_argument("--l2", type=int)
    p.add_argument("--buckets", type=int)
    p.add_argument("--n", type=int, default=1000)
    p.add_argument("--tolerance", type=int, default=0)
    p.add_argument("--seed", type=int, default=3)
    args = p.parse_args()

    if args.checkpoint:
        model = nnue.load_model(torch.load(args.checkpoint, map_location="cpu"))
    else:
        model = random_model(nnue.make_arch(args.arch, args.ft, args.l1, args.l2, args.buckets), args.seed)
    model.eval()

    rec = np.fromfile(args.data, dtype=nnue.RECORD)
    rng = np.random.default_rng(args.seed)
    if len(rec) > args.n:
        rec = rec[np.sort(rng.choice(len(rec), args.n, replace=False))]
    with tempfile.TemporaryDirectory() as tmp:
        net = args.net or os.path.join(tmp, "check.nnue")
        nnue.write_rgnn(net, nnue.quantise(model))
        qw = nnue.read_rgnn(net)  # what the engine reads, not what we meant to write
        data = os.path.join(tmp, "check.bin")
        rec.tofile(data)
        out = subprocess.run([args.engine, "evaldata", net, data], check=True, capture_output=True, text=True)
    engine = np.array([int(v) for v in out.stdout.split()], dtype=np.int64)
    python = nnue.forward_int(qw, rec)
    if len(engine) != len(rec):
        sys.exit("engine returned %d scores for %d positions" % (len(engine), len(rec)))

    diff = np.abs(engine - python)
    with torch.no_grad():
        x, xo, b = nnue.inputs(model.arch, *(torch.from_numpy(np.asarray(nnue.stm_of(rec) if k == "stm" else rec[k]).astype(np.int64))
                                             for k in ("black", "white", "kings", "stm")))
        fl = model(x, xo, b).clamp(-nnue.SCORE_LIMIT, nnue.SCORE_LIMIT).numpy()
        sim = model(x, xo, b, quant=True).clamp(-nnue.SCORE_LIMIT, nnue.SCORE_LIMIT).numpy()
    print("net %s" % (model.arch,))
    print("positions %d  distinct engine scores %d  range [%d, %d]" %
          (len(rec), len(np.unique(engine)), engine.min(), engine.max()))
    print("python int vs engine: max |diff| %d, %d positions differ" % (diff.max(), np.count_nonzero(diff)))
    print("float model vs engine: mean |diff| %.2f, max %.2f centi-men" %
          (np.abs(fl - engine).mean(), np.abs(fl - engine).max()))
    print("simulated int (training) vs engine: max |diff| %.2f" % np.abs(sim - engine).max())
    if diff.max() > args.tolerance:
        bad = np.flatnonzero(diff > args.tolerance)[:5]
        for i in bad:
            print("  record %d: engine %d python %d" % (i, engine[i], python[i]))
        sys.exit("FAIL")
    print("OK")


if __name__ == "__main__":
    main()
