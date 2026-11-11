#!/usr/bin/env bash
# WP13 targeted test of a repaired book (lead 2026-11-11): Regicide (DLL <DLLPREFIX>-<BOOK>) on the lost ballots, in the
# colour it lost with, against KingsRow mode 3 (10 games per ballot), Cake and GuiNN (4 each; NCAKE overrides Cake's
# count); fixed-colour decks, current runner, every engine "set book 3" with read-back. A repair is accepted when its
# ballot has no loss in these games.
#   wp13_test.sh BOOKTAG TRY [BALLOTS]   e.g. wp13_test.sh r10k 1   -> tags w13-t1-r10k-<opp>
#   BALLOTS: comma-separated ACF numbers (default 60,71,34,141,9), e.g. 34;  env DLLPREFIX (default bl-456e9b2)
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
B=$1; T=$2; BL=${3:-60,71,34,141,9}; DP=${DLLPREFIX:-bl-456e9b2}
st() { echo "$* $(date -Is)" >> $L/status; }
line() {  # ballot moves and Regicide's colour (the colour of the lost game)
  case $1 in
    60) echo "10-14 22-17 9-13 | A=B" ;; 71) echo "10-14 23-19 7-10 | A=B" ;; 34) echo "9-13 24-20 11-16 | A=B" ;;
    141) echo "11-16 22-17 9-13 | A=B" ;; 9) echo "9-13 22-18 11-16 | A=B" ;; 62) echo "10-14 22-17 11-16 | A=B" ;;
    93) echo "10-15 22-17 9-13 | A=B" ;; 53) echo "9-14 24-19 11-15 | A=W" ;; 55) echo "9-14 24-20 5-9 | A=W" ;;
    *) echo "unknown ballot $1" >&2; exit 2 ;;
  esac
}
mk() {  # mk N FILE
  : > $2
  for b in ${BL//,/ }; do
    m=$(line $b) || exit 2
    for ((i = 0; i < $1; i++)); do echo "| $m" >> $2; done
  done
}
mk 10 $L/wp13/test_deck_kingsrow_$T.txt; mk 4 $L/wp13/test_deck_4_$T.txt; mk ${NCAKE:-4} $L/wp13/test_deck_cake_$T.txt
st "WP13 test $T ($DP-$B, ballots $BL) start"
for opp in kingsrow cake guinn; do
  case $opp in kingsrow) deck=$L/wp13/test_deck_kingsrow_$T.txt ;; cake) deck=$L/wp13/test_deck_cake_$T.txt ;;
    *) deck=$L/wp13/test_deck_4_$T.txt ;; esac
  MATCH=$L/bin/match-cur EXTRA_ARGS=--allow-rejected-options \
    bash $J/bl_run_w.sh regicide:$DP-$B $opp w13-t$T-$B-$opp $deck 0.5 8 < /dev/null || { st "WP13 test STOPPED at w13-t$T-$B-$opp"; exit 1; }
done
st "WP13 test $T ($B) done"
