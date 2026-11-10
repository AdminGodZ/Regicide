#!/usr/bin/env bash
# Ballot reports for the conversion-search DLL runs (truth-c0dde2e).
R=~/regicide-match/results; S=~/regicide-truth/experiments/truth-data/scripts; J=~/regicide-jobs/truth
for opp in cake guinn; do
  a=$R/tr-n8bn-$opp-c0dde2e; b=$R/tr-n8c-$opp-c0dde2e
  if [ -f $b/results.csv ]; then python3 $S/ballots.py "n8c=$b" "n8bn=$a"; else python3 $S/ballots.py "n8bn=$a"; fi > $J/p1b_${opp}_ballots.md
done
python3 $S/ballots.py "c0dde2e=$R/tr-n8bn-cake-c0dde2e" "ponder-df7780a=$R/tr-n8bn-cake1,$R/tr-n8bn-cake2" > $J/p1b_cake_dll_ballots.md
cat $J/p1b_cake_ballots.md $J/p1b_guinn_ballots.md $J/p1b_cake_dll_ballots.md
