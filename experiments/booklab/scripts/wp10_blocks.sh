#!/usr/bin/env bash
# WP10 blocks 3-5 (holdout panels), current runner (master 40e1f59 tools/match + the A=B/A=W field, built in the
# booklab worktree: bin/match-cur) with --allow-rejected-options so the opponents get the same option list as in the
# final validation (rejections are logged as warnings). Settings as the final validation: 0.5 s/move CheckerBoard
# banking mode, -c 8, hash 128, one thread, Regicide's 2-8-piece db, opponents with their books and the 8-piece db.
# Configurations: O = bl-fcf6acb-deep2b, E = bl-456e9b2-deep2b, EB = bl-456e9b2-r10, EBN = EB + set evalfile n8q.
# Inside each block the configurations alternate per part and opponent, so load drifts hit them equally.
#   wp10_blocks.sh BLOCK...   (3, 4, 5)
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab; H=$L/holdout; P=$L/wp10/parts
export MATCH=$L/bin/match-cur EXTRA_ARGS=--allow-rejected-options
mkdir -p $P
grep -v "^\s*#" $H/h1_barred.txt | grep -v "^\s*$" > $P/H1.txt
grep -v "^\s*#" $H/h2_holdout156.txt | grep -v "^\s*$" > $P/H2.txt
grep -v "^\s*#" $H/h3_unresolved156.txt | grep -v "^\s*$" > $P/H3.all; split -l 78 -d -a 1 $P/H3.all $P/H3_; for f in $P/H3_?; do mv $f $f.txt; done
grep -v "^\s*#" $H/h2x_touched156.txt | grep -v "^\s*$" > $P/H2x.all; split -l 78 -d -a 1 $P/H2x.all $P/H2x_; for f in $P/H2x_?; do mv $f $f.txt; done
cfg() {  # cfg NAME -> DLL and extra option
  case $1 in
    O) DLL=bl-fcf6acb-deep2b; EA="" ;;
    E) DLL=bl-456e9b2-deep2b; EA="" ;;
    EB) DLL=bl-456e9b2-r10; EA="" ;;
    EBN) DLL=bl-456e9b2-r10; EA='set evalfile C:\regicide-match\nets\booklab\n8q.nnue' ;;
  esac
}
run_block() {  # run_block BLOCK "CONFIGS" PARTS...
  local b=$1 configs=$2; shift 2
  echo "WP10 block $b start ($configs) $(date -Is)" >> $L/status
  for part in "$@"; do for opp in cake kingsrow guinn; do for c in $configs; do
    cfg $c
    EXTRA_A="$EA" bash $J/bl_run_w.sh regicide:$DLL $opp "w10-b$b-$c-$opp-$part" $P/$part.txt 0.5 8 < /dev/null \
      || { echo "WP10 block $b FAILED at w10-b$b-$c-$opp-$part $(date -Is)" >> $L/status; exit 1; }
  done; done; done
  echo "WP10 block $b done $(date -Is)" >> $L/status
}
for b in "$@"; do
  case $b in
    3) run_block 3 "O EB" H1 H2 H3_0 H3_1 ;;
    4) run_block 4 "E EB" H2x_0 H2x_1 ;;
    5) run_block 5 "EBN" H1 H2 H3_0 H3_1 ;;
  esac
done
sleep 3
