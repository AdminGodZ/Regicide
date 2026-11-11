"""Share of the E5 games written since the last restart_e5.sh that repeat any earlier game (byte-identical) or start
from an earlier game's start (same GAME_START record), over both files. Offsets: ~/regicide-jobs/truth/restart_offset_*.txt.
Lead's criterion (2026-11-11): below 1%."""
import collections
import hashlib
import sys

import numpy as np

sys.path.insert(0, "/home/admingod/regicide-truth/tools/nnue")
import nnue  # noqa: E402

O = "/home/admingod/regicide-data/truth"
J = "/home/admingod/regicide-jobs/truth"
names = ["gen6a_n50k_egdb", "gen6b_n50k_egdb"]
off = {n: int(open("%s/restart_offset_%s.txt" % (J, n)).read().split()[1]) for n in names}
old_games, old_starts, new = set(), set(), []
for n in names:
    m = np.memmap("%s/%s.bin" % (O, n), nnue.RECORD, "r")
    m = m[:len(m)]
    starts = np.flatnonzero(np.asarray(m["stm"]) & nnue.GAME_START)
    ends = np.append(starts[1:], len(m))
    raw = np.asarray(m).view(np.uint8).reshape(-1, 16)
    for s, e in zip(starts, ends):
        if s >= off[n] and e == len(m):
            continue  # the game being written now may be incomplete
        h = hashlib.blake2b(raw[s:e].tobytes(), digest_size=16).digest()
        k = raw[s, :12].tobytes() + bytes([raw[s, 14] & 1])  # black, white, kings, side to move
        if s < off[n]:
            old_games.add(h)
            old_starts.add(k)
        else:
            new.append((s - off[n], n, h, k, e - s))
new.sort()
c = collections.Counter()
seen_g, seen_k = {}, {}
for _, n, h, k, length in new:
    if h in old_games:
        c["repeats an earlier game"] += 1
    elif k in old_starts:
        c["same start as an earlier game (different game)"] += 1
    elif h in seen_g:
        c["repeats a new game (%s file)" % ("same" if seen_g[h] == n else "other")] += 1
    elif k in seen_k:
        c["same start as a new game (%s file)" % ("same" if seen_k[k] == n else "other")] += 1
    else:
        c["new"] += 1
    seen_g.setdefault(h, n)
    seen_k.setdefault(k, n)
tot = sum(c.values())
print("games since the restart: %d (records from %s)" % (tot, ", ".join("%s %d" % (n[:5], off[n]) for n in names)))
for k, v in c.most_common():
    print("  %-50s %7d  %.2f%%" % (k, v, 100 * v / max(1, tot)))
rep = tot - c["new"]
print("repeating anything earlier (game or start): %d of %d = %.2f%%  -> %s" % (
    rep, tot, 100 * rep / max(1, tot), "below 1%" if rep < 0.01 * tot else "NOT below 1%"))
