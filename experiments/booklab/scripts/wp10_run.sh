#!/usr/bin/env bash
# WP10 holdout: each panel (openings file) against each opponent with three configurations:
#   (1) bl-fcf6acb-deep2b  old master + its own book
#   (2) bl-456e9b2-deep2b  master + the old book (engine change alone)
#   (3) bl-456e9b2-r10     master + the new book
# Usual settings (0.5 s/move, -c 8, hash 128, opponents with their books and the 8-piece db). Each panel is split
# into parts of <= 78 openings (156 games, one timing-lock hold each). Interleaved by part so that the three
# configurations see the same machine conditions.
#   wp10_run.sh TAG PANELFILE [PANELFILE...]     (env CONFIGS / OPPS override the lists)
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
TAG=$1; shift
CONFIGS=${CONFIGS:-"bl-fcf6acb-deep2b bl-456e9b2-deep2b bl-456e9b2-r10"}
OPPS=${OPPS:-"cake kingsrow guinn"}
mkdir -p $L/wp10
echo "WP10 $TAG start $(date -Is)" >> $L/status
for panel in "$@"; do
  p=$(basename $panel .txt)
  grep -v "^\s*#" $panel | grep -v "^\s*$" > $L/wp10/$p.all
  split -l 78 -d -a 1 $L/wp10/$p.all $L/wp10/$p.part
  for part in $L/wp10/$p.part*; do
    k=${part##*.part}
    for opp in $OPPS; do for c in $CONFIGS; do
      bash $J/bl_run.sh regicide:$c $opp "w10-$TAG-$p-$c-$opp-$k" $part 0.5 8 < /dev/null
    done; done
  done
done
echo "WP10 $TAG done $(date -Is)" >> $L/status
sleep 3
