#!/usr/bin/env bash
# P2 / E3: n8c = n8bn's recipe (relabelled results, >= 9 pieces, n6c's K 102.4) with gen5a (x3) and gen4a rescored
# (proven records trained on their exact result). Match vs n6c at 0.1 s; if its Elo beats n8bn's +4.5 (point
# estimate), also n8c vs n8bn directly (805 pairs, 0.1 s).
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data/truth; W=~/regicide-truth-work
until [ -s $O/gen5a_n50k_e3.bin ] && [ -s $O/gen4a_n10k_e3.bin ]; do sleep 60; done
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d; do ALL="$ALL --data $O/${f}_n10k_e2.bin"; done
ALL="$ALL --data $O/gen4a_n10k_e3.bin"
G5="--data $O/gen5a_n50k_e3.bin"
K=$(python3 -c "import json; print(json.load(open('$HOME/regicide-nnue6-work/runs/n6c/config.json'))['K'])")
bash /mnt/c/regicide-match/jobs/truth/train.sh n8c $ALL $G5 $G5 $G5 --lambda 0.5 --min-pieces 9 --k $K || { st "CHAIN3 stopped: n8c export check missing"; exit 1; }
bash /mnt/c/regicide-match/jobs/truth/match.sh n8c 0.1
e=$(grep -o '^Elo (pentanomial, pairs): [-+0-9.]*' $W/matches/n8c-vs-n6c/summary.txt | awk '{print $4}')
st "n8c vs n6c Elo $e (n8bn +4.5)"
if python3 -c "import sys; sys.exit(0 if float('$e') > 4.5 else 1)"; then
  st "match n8c-vs-n8bn queued"
  with_timing selfplay_match n8c $W/n8c.nnue $W/matches/n8c-vs-n8bn $W/engines/rtruth.so 0.1 n8bn $W/n8bn.nnue
  st "match n8c-vs-n8bn end: $(grep '^Elo (pent' $W/matches/n8c-vs-n8bn/summary.txt)"
fi
st "CHAIN3_DONE"
