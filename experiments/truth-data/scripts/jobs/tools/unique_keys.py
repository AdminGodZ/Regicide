"""Diversity of training files: records, records left after removing exactly repeated games ("game-deduplicated"),
and unique board+side-to-move keys (black, white, kings, side to move). Also, per flagged file, the share of validation
positions (tail from valid_start) whose key also occurs in that file's training part."""
import hashlib
import sys

import numpy as np

sys.path.insert(0, "/home/admingod/regicide-truth/tools/nnue")
import nnue  # noqa: E402


def keys(rec):
    rec = np.asarray(rec)
    k = np.empty((len(rec), 2), np.uint64)
    k[:, 0] = rec["black"].astype(np.uint64) | (rec["white"].astype(np.uint64) << np.uint64(32))
    k[:, 1] = rec["kings"].astype(np.uint64) | ((rec["stm"].astype(np.uint64) & np.uint64(1)) << np.uint64(32))
    return k.view(np.dtype((np.void, 16))).ravel()


def game_dedup_records(m):
    flags = np.asarray(m["stm"]) & nnue.GAME_START
    if not len(m) or not flags[0]:
        return None
    starts = np.flatnonzero(flags)
    ends = np.append(starts[1:], len(m))
    raw = np.asarray(m).view(np.uint8).reshape(-1, 16)
    seen, kept = set(), 0
    for s, e in zip(starts, ends):
        h = hashlib.blake2b(raw[s:e].tobytes(), digest_size=16).digest()
        if h not in seen:
            seen.add(h)
            kept += e - s
    return kept


groups = {}
for arg in sys.argv[1:]:
    name, files = arg.split("=", 1)
    groups[name] = files.split(",")
for name, files in groups.items():
    maps = [np.memmap(f, nnue.RECORD, "r") for f in files]
    total = sum(len(m) for m in maps)
    allk = np.unique(np.concatenate([np.unique(keys(m)) for m in maps]))
    dd = [game_dedup_records(m) for m in maps]
    print("%s: records %d; game-deduplicated %s; unique board+side keys %d (%.1f%% of records)" % (
        name, total, "%d" % sum(dd) if all(x is not None for x in dd) else "n/a (no game flags)", len(allk),
        100 * len(allk) / total))
    for f, m in zip(files, maps):
        s = nnue.valid_start(m, 0.02)
        kv = keys(m[s:])
        kt = np.unique(keys(m[:s]))
        both = np.isin(kv, kt)
        print("  %s: validation %d positions from record %d; also in training: %d (%.1f%%)" % (
            f.split("/")[-1], len(kv), s, both.sum(), 100 * both.mean()))
