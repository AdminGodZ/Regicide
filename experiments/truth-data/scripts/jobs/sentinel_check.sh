#!/usr/bin/env bash
# Review issue 1: could already-rescored files hold sentinel "proven losses"? A sentinel maps to score -3000 + PROVEN,
# exactly like a real proven loss, so the value alone cannot tell. Two checks:
#  (a) the guarded rescore (rescore-tr2) over a sample at the production budgets: count of untrusted searches;
#  (b) in every rescored file: proven losses whose previous (E2) score was > +100 (what a sentinel on a position the
#      old search liked would look like), against proven wins with previous score < -100 (the symmetric control,
#      which a sentinel cannot produce).
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data/truth; T=/tmp/sentinel; mkdir -p $T
head -c $((300000 * 16)) $O/gen4a_n10k_e2.bin > $T/s4.bin
head -c $((200000 * 16)) $O/gen5a_n50k_e2.bin > $T/s5.bin
rm -f $T/o4.bin $T/o5.bin
cp ~/regicide-truth/build/rescore $J/bin/rescore-tr3 2>/dev/null
$J/bin/rescore-tr3 --egdb ~/regicide-data/egdb --in $T/s4.bin --out $T/o4.bin --threads 2 --nodes 9:50000,13:20000 --keep-above 16 | tail -n 2 > $J/sentinel_check.txt
$J/bin/rescore-tr3 --egdb ~/regicide-data/egdb --in $T/s5.bin --out $T/o5.bin --threads 2 --nodes 9:50000 --keep-above 16 | tail -n 2 >> $J/sentinel_check.txt
~/regicide-nnue/.venv/bin/python - >> $J/sentinel_check.txt <<'PY'
import os, sys; sys.path.insert(0, "/home/admingod/regicide-truth/tools/nnue"); import nnue, numpy as np
O = os.path.expanduser("~/regicide-data/truth")
pairs = [("gen5a_n50k_e2.bin", "gen5a_n50k_e3.bin"), ("gen4a_n10k_e2.bin", "gen4a_n10k_e3.bin.part")]
for f in ("gen5a_n50k", "gen5c_n50k", "gen4a_n10k"):
    pairs.append(("tails/tail_%s_9_12.bin" % f, "tails/tail_%s_9_12_rs.bin" % f))
for a, b in pairs:
    pb = os.path.join(O, b)
    if not os.path.exists(pb): continue
    y = np.memmap(pb, nnue.RECORD, "r"); x = np.memmap(os.path.join(O, a), nnue.RECORD, "r")[: len(y)]
    pr = nnue.proven_of(y); loss = pr & (y["score"] < 0); win = pr & (y["score"] > 0)
    print("%s: records %d proven %d (losses %d, wins %d); proven losses with old score > +100: %d; proven wins with old score < -100: %d" % (
        b, len(y), pr.sum(), loss.sum(), win.sum(), (loss & (x["score"] > 100)).sum(), (win & (x["score"] < -100)).sum()))
PY
st "sentinel check: $(head -n 4 $J/sentinel_check.txt | grep -o 'untrusted [0-9]*' | tr '\n' ' ')"
