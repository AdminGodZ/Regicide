#!/usr/bin/env bash
# WP13 third source: Regicide's own 20M-node search (book off, cleared hash per move, last completed iteration) of
# every legal move at each of Regicide's positions (Black to move) on the lost lines against KingsRow mode 3, from
# the end of the ballot to ply 40. Lines: one lost game each of ACF #60, #71, #34, #141 (= #9's line).
# Output ~/regicide-jobs/booklab/wp13/score20m.tsv (id = ballot:ply, then multiscore's columns).
L=~/regicide-jobs/booklab; R=~/regicide-match/results; W=~/regicide-booklab
mkdir -p $L/wp13; cd $L/wp13
python3 - > points.tsv <<'PY'
import sys
sys.path.insert(0, '/home/admingod/regicide-booklab/tools/booklab')
from theory import parse_pdn, family, ballot_of
R = '/home/admingod/regicide-match/results'
want = {('bl-k3-kingsrow-a-h1', '52.1'), ('bl-k3-kingsrow-a-h1', '63.1'), ('bl-k3-kingsrow-b-h1', '31.1'),
        ('bl-k3-kingsrow-a-h2', '49.1')}
for run, rnd in sorted(want):
    for tags, mv in parse_pdn(open(f'{R}/{run}/games.pdn').read()):
        if tags['Round'] == rnd:
            print(f"{ballot_of(tags['Opening'])[0].split()[1]}\t{' '.join(m for m, _ in mv)}")
PY
# FEN of every Black-to-move position from ply 3 (after the ballot) to ply 40, by replaying the move list
: > fens.tsv
while IFS=$'\t' read b moves; do
  arr=($moves)
  for ((i = 4; i < 40 && i < ${#arr[@]}; i += 2)); do   # i plies played: Black (Regicide) to move
    fen=$(printf '0\t\t%s\n' "${arr[*]:0:$i}" | $W/build-linux/booklab_replay | awk -F'\t' '$1=="Z"{print $4}')
    printf '%s:%d\t%s\n' "$b" $((i + 1)) "$fen" >> fens.tsv
  done
done < points.tsv
wc -l < fens.tsv
nice $L/bin/blk-multiscore 20000000 6 ~/regicide-data/egdb 128 $L/book_r10.bin < fens.tsv > score20m.tsv
echo "WP13 20M-node scores done ($(wc -l < score20m.tsv) moves) $(date -Is)" >> $L/status
