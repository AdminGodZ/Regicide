#!/usr/bin/env bash
# WP13 targeted test of a repaired book (lead 2026-11-11): Regicide (DLL bl-456e9b2-<BOOK>) as Black on the lost
# ballots against KingsRow mode 3 (10 games per ballot), Cake and GuiNN (4 each); fixed-colour decks, current runner,
# every engine "set book 3" with read-back. A repair is accepted when its ballot has no loss in these games.
#   wp13_test.sh BOOKTAG TRY [BALLOTS]   e.g. wp13_test.sh r10k 1   -> tags w13-t1-r10k-<opp>
#   BALLOTS: comma-separated ACF numbers (default 60,71,34,141,9), e.g. 34
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
B=$1; T=$2; BL=${3:-60,71,34,141,9}
st() { echo "$* $(date -Is)" >> $L/status; }
moves() {
  case $1 in
    60) echo "10-14 22-17 9-13" ;; 71) echo "10-14 23-19 7-10" ;; 34) echo "9-13 24-20 11-16" ;;
    141) echo "11-16 22-17 9-13" ;; 9) echo "9-13 22-18 11-16" ;; *) echo "unknown ballot $1" >&2; exit 2 ;;
  esac
}
mk() {  # mk N FILE
  : > $2
  for b in ${BL//,/ }; do
    m=$(moves $b) || exit 2
    for ((i = 0; i < $1; i++)); do echo "| $m | A=B" >> $2; done
  done
}
mk 10 $L/wp13/test_deck_kingsrow_$T.txt; mk 4 $L/wp13/test_deck_4_$T.txt
st "WP13 test $T ($B, ballots $BL) start"
for opp in kingsrow cake guinn; do
  deck=$L/wp13/test_deck_4_$T.txt; [ $opp = kingsrow ] && deck=$L/wp13/test_deck_kingsrow_$T.txt
  MATCH=$L/bin/match-cur EXTRA_ARGS=--allow-rejected-options \
    bash $J/bl_run_w.sh regicide:bl-456e9b2-$B $opp w13-t$T-$B-$opp $deck 0.5 8 < /dev/null || { st "WP13 test STOPPED at w13-t$T-$B-$opp"; exit 1; }
done
st "WP13 test $T ($B) done"
