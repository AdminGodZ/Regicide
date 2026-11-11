"""Regression test (review r17, finding 2): a positive --valid-frac that leaves no validation records must stop the
trainer instead of printing "valid 0.000000"; one-game flagged file -> the cut moves to the end -> 0 records."""
import os
import subprocess
import sys
import tempfile

import numpy as np

T = os.path.join(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, T)
import nnue  # noqa: E402

with tempfile.TemporaryDirectory() as d:
    rec = np.zeros(200, nnue.RECORD)
    rec["black"] = 0x00000FFF
    rec["white"] = 0xFFF00000
    rec["stm"] = 0
    rec["stm"][0] |= nnue.GAME_START  # one game of 200 records: no game start inside the 2% tail
    f = os.path.join(d, "one_game.bin")
    rec.tofile(f)
    m = np.memmap(f, nnue.RECORD, "r")
    assert nnue.valid_start(m, 0.02) == len(m)
    data = nnue.Data([f], 0.02)
    assert data.valid_counts == [0] and data.n_valid == 0, data.valid_counts
    r = subprocess.run([sys.executable, os.path.join(T, "train.py"), "--data", f, "--out", os.path.join(d, "run"),
                        "--epochs", "1", "--device", "cpu"], capture_output=True, text=True)
    assert r.returncode != 0 and "leaves no validation records" in r.stderr, (r.returncode, r.stdout[-500:], r.stderr[-500:])
    assert "validation records 0" in r.stdout and "the cut moved to the end" in r.stdout, r.stdout[-500:]
print("valid_test: a valid-frac that leaves no validation records stops the trainer")
