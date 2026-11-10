#!/usr/bin/env bash
# P4 throughput test: gen5a-style datagen (no database, random openings, 50k nodes) against the E5 settings (8-piece
# database adjudication, proven positions kept, half the games from real-game seeds + 2-4 random plies), same engine,
# 3 threads each, side by side for 20 minutes. Reports CPU time and positions (all and >= 9 pieces) per CPU-hour.
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data/truth; B=$J/bin; cp ~/regicide-truth/build/datagen $B/datagen-tr5
rm -f $O/e5test_old.bin $O/e5test_new.bin
st "e5 rate test start"
$B/datagen-tr5 --games 1000000 --nodes 50000 --threads 3 --seed 901 --out $O/e5test_old.bin > $J/e5test_old.log 2>&1 &
P1=$!
$B/datagen-tr5 --games 1000000 --nodes 50000 --threads 3 --seed 902 --egdb ~/regicide-data/egdb --seeds $O/seeds_real_6_30.txt --seed-frac 0.5 --seed-plies 2-4 --out $O/e5test_new.bin > $J/e5test_new.log 2>&1 &
P2=$!
cpu() { awk '{print ($14 + $15) / 100}' /proc/$1/stat; }
sleep 1200
c1=$(cpu $P1); c2=$(cpu $P2); kill $P1 $P2; wait
for t in old new; do
  c=$([ $t = old ] && echo $c1 || echo $c2)
  ~/regicide-nnue/.venv/bin/python - $O/e5test_$t.bin $c $t >> $J/e5_rate.txt <<'PY'
import sys; sys.path.insert(0, "/home/admingod/regicide-truth/tools/nnue"); import nnue, numpy as np
r = np.fromfile(sys.argv[1], nnue.RECORD); cpu = float(sys.argv[2]); p = nnue.piece_counts(r); pr = nnue.proven_of(r)
print("%s: cpu %.0f s, records %d (%.0f/cpu-h), >=9 pieces %d (%.0f/cpu-h), proven %d, <=8 %d" % (
    sys.argv[3], cpu, len(r), len(r) / cpu * 3600, (p >= 9).sum(), (p >= 9).sum() / cpu * 3600, pr.sum(), (p <= 8).sum()))
PY
done
st "e5 rate test end: $(tail -n 2 $J/e5_rate.txt | tr '\n' ' ')"
