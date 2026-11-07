"""Checks that an exported net evaluates identically in Python and in the engine.

  python tools/nnue/check_export.py --engine build/regicide --data positions.bin [--checkpoint last.pt] [--n 1000]

Exports the checkpoint (or a random net with weights spread over the quantisation ranges) to RGNN, evaluates
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


def random_model(seed):
    torch.manual_seed(seed)
    m = nnue.Net()
    with torch.no_grad():
        m.ft.weight.uniform_(-1.2, 1.2)
        m.ft.bias.uniform_(-0.5, 1.0)
        m.l1.weight.uniform_(-0.4, 0.4)
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
    p.add_argument("--n", type=int, default=1000)
    p.add_argument("--tolerance", type=int, default=0)
    p.add_argument("--seed", type=int, default=3)
    args = p.parse_args()

    if args.checkpoint:
        ck = torch.load(args.checkpoint, map_location="cpu")
        model = nnue.Net(ck["args"].get("out_scale", nnue.OUT_SCALE))
        model.load_state_dict(ck["model"])
    else:
        model = random_model(args.seed)
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
        x, b = nnue.features(*(torch.from_numpy(rec[k].astype(np.int64)) for k in ("black", "white", "kings", "stm")))
        fl = model(x, b).clamp(-nnue.SCORE_LIMIT, nnue.SCORE_LIMIT).numpy()
        sim = model(x, b, quant=True).clamp(-nnue.SCORE_LIMIT, nnue.SCORE_LIMIT).numpy()
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
