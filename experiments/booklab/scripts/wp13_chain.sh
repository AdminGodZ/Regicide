#!/usr/bin/env bash
# WP13 (lead 2026-11-10 21:30): repair the KingsRow mode-3 losses.
#   1. Evidence: KingsRow in book mode 3 as White against Cake, GuiNN and KingsRow mode 3 (a second copy) as Black on
#      the lost ballots (ACF #60, #71, #34, #141, #9; #141 and #9 share the line), 8 games per ballot and opponent
#      (fixed colour, current runner, every engine "set book 3" with read-back): tags w13-ev-<black>.
#   2. Census: Cake against KingsRow mode 3 on the production deck (312 games, final-validation runner):
#      tags c3-cake-kingsrow-h1/h2.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
st() { echo "$* $(date -Is)" >> $L/status; }
mkdir -p $L/wp13
DECK=$L/wp13/evidence_deck.txt
: > $DECK
for b in "10-14 22-17 9-13" "10-14 23-19 7-10" "9-13 24-20 11-16" "11-16 22-17 9-13" "9-13 22-18 11-16"; do
  for i in 1 2 3 4 5 6 7 8; do echo "| $b | A=W" >> $DECK; done
done
st "WP13 evidence start"
for black in cake guinn kingsrow2; do
  MATCH=$L/bin/match-cur EXTRA_ARGS=--allow-rejected-options \
    bash $J/bl_run_w.sh kingsrow $black w13-ev-$black $DECK 0.5 8 < /dev/null || { st "WP13 STOPPED at w13-ev-$black"; exit 1; }
done
st "WP13 evidence done"
for h in h1 h2; do
  bash $J/bl_run_x.sh cake kingsrow c3-cake-kingsrow-$h $J/acf156_$h.txt < /dev/null || { st "WP13 STOPPED at c3-cake-kingsrow-$h"; exit 1; }
done
st "WP13 census done"
