#!/usr/bin/env bash
# WP13 targeted test of a repaired book (lead 2026-11-11): Regicide (DLL bl-456e9b2-<BOOK>) as Black on the lost
# ballots ACF #60, #71, #34, #141, #9 against KingsRow mode 3 (10 games per ballot), Cake and GuiNN (4 each);
# fixed-colour decks, current runner, every engine "set book 3" with read-back. A repair is accepted when its ballot
# has no loss in these games.
#   wp13_test.sh BOOKTAG TRY      e.g. wp13_test.sh r10k 1   -> tags w13-t1-r10k-<opp>
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
B=$1; T=$2
st() { echo "$* $(date -Is)" >> $L/status; }
mk() {  # mk N FILE
  : > $2
  for b in "10-14 22-17 9-13" "10-14 23-19 7-10" "9-13 24-20 11-16" "11-16 22-17 9-13" "9-13 22-18 11-16"; do
    for ((i = 0; i < $1; i++)); do echo "| $b | A=B" >> $2; done
  done
}
mk 10 $L/wp13/test_deck_kingsrow.txt; mk 4 $L/wp13/test_deck_4.txt
st "WP13 test $T ($B) start"
for opp in kingsrow cake guinn; do
  deck=$L/wp13/test_deck_4.txt; [ $opp = kingsrow ] && deck=$L/wp13/test_deck_kingsrow.txt
  MATCH=$L/bin/match-cur EXTRA_ARGS=--allow-rejected-options \
    bash $J/bl_run_w.sh regicide:bl-456e9b2-$B $opp w13-t$T-$B-$opp $deck 0.5 8 < /dev/null || { st "WP13 test STOPPED at w13-t$T-$B-$opp"; exit 1; }
done
st "WP13 test $T ($B) done"
