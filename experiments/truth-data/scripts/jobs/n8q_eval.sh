#!/usr/bin/env bash
# T4: judge n8q (and n8q5 if n8q costs anything against n8bn).
#  (a) NAME vs n8bn with the database, 0.1 s, 805 pairs (rtruth.so, fcf6acb);
#  (b) NAME vs n6c and n8bn vs n6c WITHOUT the database (the same .so in engines-nodb/: dbpieces 0), same openings;
#  (c) static evals on <= 8-piece positions (sanity.sh).
source /mnt/c/regicide-match/jobs/truth/common.sh
W=~/regicide-truth-work; T=/mnt/c/regicide-match/jobs/truth; SO=$W/engines/rtruth.so; ND=$W/engines-nodb/rtruth.so
h2h() { st "match $1-vs-$2 queued"; with_timing selfplay_match $1 $W/$1.nnue $W/matches/$1-vs-$2 $SO 0.1 $2 $W/$2.nnue; st "match $1-vs-$2 end: $(grep '^Elo (pent' $W/matches/$1-vs-$2/summary.txt)"; }
nodb() { st "match $1-vs-n6c-nodb queued"; with_timing selfplay_match $1 $W/$1.nnue $W/matches/$1-vs-n6c-nodb $ND 0.1; st "match $1-vs-n6c-nodb end: $(grep '^Elo (pent' $W/matches/$1-vs-n6c-nodb/summary.txt)"; }
elo() { grep -o '^Elo (pentanomial, pairs): [-+0-9.]*' $1 | awk '{print $4}'; }
eval_net() {
  until [ -s $W/$1.ok ]; do grep -q "train $1 EXPORT CHECK FAILED" $J/status && return 1; sleep 30; done
  bash $T/sanity.sh $J/sanity_$1.md n6c=builtin n8bn=$W/n8bn.nnue $1=$W/$1.nnue > /dev/null
  st "sanity $1: $(grep '3 kings' $J/sanity_$1.md | head -n 1)"
  h2h $1 n8bn
  nodb $1
}
eval_net n8q || { st "T4: n8q failed"; exit 1; }
nodb n8bn
if python3 -c "import sys; sys.exit(0 if float('$(elo $W/matches/n8q-vs-n8bn/summary.txt)') < 0 else 1)"; then
  bash $T/n8q.sh n8q5 1,19 && eval_net n8q5
fi
st "T4_DONE"
