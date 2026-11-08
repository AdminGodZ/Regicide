#!/usr/bin/env bash
# Run inside an attached wsl.exe. Book/avoid CheckerBoard tests on master 769ef0b DLLs with the 2-8 egdb and the EGDB
# setting chosen by switch769.sh (~/regicide-jobs/ba/egdb_setting: empty or "set dbmbytes 12288"); baseline settings
# (acf156, 312 games, 0.5 s/move, -c 8). Tags end in -769.
#   Test A: off vs avoid (set avoidfile avoid_v1.bin), interleaved per opponent;
#   Test B: book+avoid (DLL 769ef0b-bookavoid-egdb8: book_n4c with avoid_v1 merged, embedded; default book mode);
#   then KingsRow at 2 s/move with the better of avoid / book+avoid (mean Elo over the three opponents).
# Background: gen5yield -> exactly one generator runs alongside (bookgen until the book is done, then gen5).
J=~/regicide-jobs/r4; JB=~/regicide-jobs/ba; R=/mnt/c/regicide-match/jobs/ba/run_ba2.sh
st() { echo "$* $(date -Is)" >> $J/status; }
until grep -q '^SWITCH769_DONE' $J/status; do sleep 10; done
while [ -e $J/matchlock ] || [ -e $J/baselock ]; do sleep 10; done
touch $J/gen5yield
EG=$(cat $JB/egdb_setting)
st "BA chain2 start 769ef0b (egdb setting '${EG:-default}')"
AV='set avoidfile C:\regicide-match\avoid\avoid_v1.bin'
for opp in kingsrow cake guinn; do
  bash $R 769ef0b-egdb8 off-769 $opp 0.5 "$EG"
  bash $R 769ef0b-egdb8 avoid-769 $opp 0.5 "$EG" "$AV"
done
st "BA_TESTA_769_DONE"
until grep -q '^BA_BOOKAVOID_DLL_DONE' $J/status; do sleep 30; done
for opp in kingsrow cake guinn; do bash $R 769ef0b-bookavoid-egdb8 bookavoid-769 $opp 0.5 "$EG"; done
st "BA_TESTB_769_DONE"
elo() { grep '^Elo (pent' ~/regicide-match/results/ba-$1-$2/summary.txt | sed -E 's/^Elo \(pentanomial, pairs\): ([-+0-9.]+) .*/\1/'; }
mean() { echo "$(elo $1 kingsrow) $(elo $1 cake) $(elo $1 guinn)" | awk '{printf "%.2f", ($1+$2+$3)/3}'; }
MA=$(mean avoid-769); MB=$(mean bookavoid-769)
if awk "BEGIN{exit !($MB > $MA)}"; then BEST=bookavoid-769; DLL=769ef0b-bookavoid-egdb8; X=""; else BEST=avoid-769; DLL=769ef0b-egdb8; X=$AV; fi
st "BA best variant: $BEST (mean Elo avoid $MA, bookavoid $MB, off $(mean off-769))"
bash $R $DLL $BEST kingsrow-2s 2 "$EG" "$X"
st "BA_ALL_769_DONE"
