#!/usr/bin/env bash
# T4(c): static evals of nets on <= 8-piece positions (no database involved): three kings against one, and 400 random
# 4-6-piece positions of gen5a (E2) with their database values (side to move): mean eval per value, share of wins
# scored > +100, of losses < -100, of draws within +-50; plus the first 6 wins with >= 2 more pieces and 6 equal-material
# draws of the sample. sanity.sh OUT NAME=NET... (NET: a file or builtin)
E=~/regicide-truth/build/regicide; out=$1; shift
F=/tmp/fens46.txt
[ -s $F ] || ~/regicide-truth/build/truth fens --egdb ~/regicide-data/egdb --in ~/regicide-data/truth/gen5a_n50k_e2.bin --min 4 --max 6 --n 400 --seed 5 > $F
python3 - "$E" "$F" "$@" > $out <<'PY'
import subprocess, sys
E, F, nets = sys.argv[1], sys.argv[2], sys.argv[3:]
rows = [l.split() for l in open(F)]
special = ["B:WK20:BK6,K9,K11", "W:WK20:BK6,K9,K11"]
def material(fen):  # side to move minus opponent, kings counted as 1
    stm, w, b = fen.split(":")
    nw, nb = len(w[1:].split(",")) if len(w) > 1 else 0, len(b[1:].split(",")) if len(b) > 1 else 0
    return (nb - nw) if stm == "B" else (nw - nb)
picked_w = [f for f, v in rows if v == "win" and material(f) >= 2][:6]
picked_d = [f for f, v in rows if v == "draw" and material(f) == 0][:6]
fens = special + [f for f, _ in rows] + picked_w + picked_d
ev = {}
for spec in nets:
    name, path = spec.split("=", 1)
    # a net that cannot be loaded must stop the table: the engine would go on with its built-in net (2026-11-11: a
    # literal "$HOME/..." path gave three identical columns)
    if path != "builtin":
        import os
        if not os.path.isfile(path):
            sys.exit("sanity.sh: %s: no such net file %s" % (name, path))
    cmds = "set evalfile %s\n" % path + "".join("evalpos %s\n" % f for f in fens) + "quit\n"
    res = subprocess.run([E], input=cmds, capture_output=True, text=True)
    out = res.stdout
    bad = [l for l in (res.stdout + res.stderr).splitlines() if any(w in l.lower() for w in ("cannot", "error", "fail"))]
    if bad:
        sys.exit("sanity.sh: %s: the engine did not take %s: %s" % (name, path, bad[0]))
    vals = [int(l.split()[1]) for l in out.splitlines() if l.startswith("eval ")]
    assert len(vals) == len(fens), (name, len(vals))
    ev[name] = dict(zip(range(len(fens)), vals))
names = [s.split("=")[0] for s in nets]
print("| position | database (stm) | " + " | ".join(names) + " |")
print("|---|---|" + "---|" * len(names))
for i, f in enumerate(special):
    print("| `%s` (3 kings vs 1) | %s | %s |" % (f, "win" if f[0] == "B" else "loss", " | ".join(str(ev[n][i]) for n in names)))
base = len(special)
for kind, lab in (("win", "wins"), ("draw", "draws"), ("loss", "losses")):
    idx = [base + j for j, (_, v) in enumerate(rows) if v == kind]
    cells = []
    for n in names:
        vs = [ev[n][i] for i in idx]
        m = sum(vs) / len(vs)
        if kind == "win": share = "%.0f%% > +100" % (100 * sum(v > 100 for v in vs) / len(vs))
        elif kind == "loss": share = "%.0f%% < -100" % (100 * sum(v < -100 for v in vs) / len(vs))
        else: share = "%.0f%% within ±50" % (100 * sum(abs(v) <= 50 for v in vs) / len(vs))
        cells.append("mean %+.0f, %s" % (m, share))
    print("| %d random 4-6-piece database %s | %s | %s |" % (len(idx), lab, kind, " | ".join(cells)))
off = base + len(rows)
for j, f in enumerate(picked_w + picked_d):
    print("| `%s` | %s | %s |" % (f, "win" if j < len(picked_w) else "draw", " | ".join(str(ev[n][off + j]) for n in names)))
PY
rc=$?
cat $out
exit $rc
