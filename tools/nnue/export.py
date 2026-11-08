"""Exports a training checkpoint as an RGNN net for the engine.

  python tools/nnue/export.py runs/net1/last.pt net.nnue
"""

import os
import sys

import torch

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import nnue  # noqa: E402


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    ck = torch.load(sys.argv[1], map_location="cpu")
    model = nnue.load_model(ck)
    nnue.write_rgnn(sys.argv[2], nnue.quantise(model))
    print("wrote %s (%s, epoch %d)" % (sys.argv[2], model.arch, ck["epoch"]))


if __name__ == "__main__":
    main()
