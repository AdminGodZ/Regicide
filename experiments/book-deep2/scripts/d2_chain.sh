#!/usr/bin/env bash
# Run inside an attached wsl.exe (Start-Process hidden). Deep book pass 2 (master 6bb8c6c = n6c + book_deep):
#  1. lossprobe (30 s/position, 6 threads x 2 jobs, 2-8 EGDB) on the book-on Cake losses (deep-on{,2,3}-cake: all on the
#     5 stubborn ballots); only confident blames (--drop 50 --margin 40) -> avoid2.bin
#  2. book2: bookgen --roots roots2.txt (the 5 ballots) --nodes 150000000 --threads 20: calibration (80 expansions),
#     then --resume with N expansions so that book2 takes ~4.5 h in all (gen5c yields to bookgen)
#  3. merge_books.py book_deep.bin + book2.bin (deeper entry per position+move wins), then
#     bookgen --resume merged --roots roots.txt (all 46) --avoid avoid2.bin --expansions 0 -> book_deep2.bin
#  4. DLL 6bb8c6c-deep2-egdb8 (book_deep2 embedded); the reference is the master DLL 6bb8c6c-egdb8 (book_deep)
#  5. targeted: the 5 ballots x 10 per run (100 games), deep2 / deep interleaved, 3 runs each (300 games per side);
#     then a full acf156 run vs Cake and one vs KingsRow with deep2. gen5c alongside.
J=~/regicide-jobs/r4; J2=~/regicide-jobs/deep2; JD=~/regicide-jobs/deep; BK=~/regicide-data/book
BG=$J2/bin/bookgen-6bb8c6c; LP=$J2/bin/lossprobe-6bb8c6c; R=/mnt/c/regicide-match/jobs/deep2/run_ba3.sh
st() { echo "D2 $* $(date -Is)" >> $J/status; }
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
ts() { while IFS= read -r l; do printf '%s %s\n' "$(date +%s)" "$l"; done; }
while [ -e $J/matchlock ] || [ -e $J/baselock ]; do sleep 20; done
st "chain start"
# roots2 + targeted openings
grep -E '^(ACF #7 |ACF #153 Black Widow|ACF #46 Double Cross|ACF #68 White Doctor|ACF #16 Wilderness II)' $JD/roots_ballots.txt > $J2/ballots2.txt
awk '{print $NF}' $J2/ballots2.txt > $J2/roots2.txt
: > $J2/target_openings.txt
for i in $(seq 10); do awk '{for(i=1;i<=NF;i++) if ($i ~ /^[0-9]+[-x][0-9]+$/) {printf "%s ", $i; n++; if (n==3) {print ""; n=0}}}' $J2/ballots2.txt >> $J2/target_openings.txt; done
sed -i 's/ *$//' $J2/target_openings.txt
st "roots2: $(wc -l < $J2/roots2.txt) roots; targeted openings $(wc -l < $J2/target_openings.txt) lines ($(sort -u $J2/target_openings.txt | tr '\n' '|'))"
# 1. lossprobe
if ! grep -q '^D2 lossprobe end rc=0' $J/status; then
  : > $J2/losses_bookon.pdn
  for r in deep-on-cake deep-on2-cake deep-on3-cake; do
    python3 - ~/regicide-match/results/ba-$r >> $J2/losses_bookon.pdn <<'PY'
import csv, sys, os
src = sys.argv[1]
for r in csv.DictReader(open(os.path.join(src, "results.csv"))):
    if r["score_a"] == "0":
        print(open(os.path.join(src, "games", f"{int(r['game']):05d}.pdn")).read().rstrip("\n") + "\n")
PY
  done
  st "lossprobe start: $(grep -c '^\[Event' $J2/losses_bookon.pdn) games, movetime 30000, threads 6, jobs 2, drop 50, margin 40"
  $LP --pdn $J2/losses_bookon.pdn --movetime 30000 --threads 6 --jobs 2 --hash 1024 --egdb ~/regicide-data/egdb \
      --drop 50 --margin 40 --out $J2/avoid2.bin --report $J2/report2.txt > $J2/lossprobe.log 2>&1
  st "lossprobe end rc=$? $(tail -n 1 $J2/lossprobe.log)"
fi
# 2. book2
TOTAL=$(( 270 * 60 ))
if [ ! -e $BK/book2.bin ]; then
  T0=$(date +%s)
  st "book2 calibration start: --roots roots2.txt --nodes 150000000 --expansions 80 --threads 20"
  stdbuf -oL $BG --roots $J2/roots2.txt --nodes 150000000 --expansions 80 --threads 20 --out $BK/book2.bin 2>&1 | ts > $J2/calib.log
  T1=$(date +%s)
  SA=$(grep ' roots analysed' $J2/calib.log | head -n 1 | awk '{print $1}')
  read E1 S1 <<< "$(grep ' expanded ' $J2/calib.log | tail -n 1 | awk '{print $3, $1}')"
  RATE=$(awk "BEGIN{d=$S1-$SA; print (d>0) ? $E1/d : 0.01}")
  N=$(awk "BEGIN{n=int($RATE*($TOTAL-($T1-$T0))); if (n<80) n=80; print n}")
  echo $N > $J2/main_expansions
  st "book2 calibration done in $((T1-T0)) s (roots analysed after $((SA-T0)) s; $E1 expansions in $((S1-SA)) s = $RATE/s) -> main --expansions $N"
fi
if ! grep -q '^D2 book2 main end rc=0' $J/status; then
  N=$(cat $J2/main_expansions); cp $BK/book2.bin $BK/book2.bin.calib
  st "book2 main start: --resume book2.bin --roots roots2.txt --nodes 150000000 --expansions $N --threads 20"
  stdbuf -oL $BG --resume $BK/book2.bin --roots $J2/roots2.txt --nodes 150000000 --expansions $N --threads 20 --out $BK/book2.bin 2>&1 | ts > $J2/main.log
  st "book2 main end rc=${PIPESTATUS[0]} $(tail -n 1 $J2/main.log | cut -d' ' -f2-)"
fi
# 3. merge + avoid
python3 /mnt/c/regicide-match/jobs/deep2/merge_books.py $BK/book_deep.bin $BK/book2.bin $BK/book_deep2_merged.bin > $J2/merge.log 2>&1
st "merge: $(cat $J2/merge.log)"
AV=(); [ -s $J2/avoid2.bin ] && AV=(--avoid $J2/avoid2.bin)
$BG --resume $BK/book_deep2_merged.bin --roots $JD/roots.txt "${AV[@]}" --expansions 0 --nodes 150000000 --threads 20 \
    --out $BK/book_deep2.bin > $J2/finalize.log 2>&1
st "book_deep2: rc=$? $(grep -E 'avoid list|resumed|wrote' $J2/finalize.log | tr '\n' ' ')"
# 4. DLL
SHA=6bb8c6c OUT=6bb8c6c-deep2-egdb8 BOOKFILE=$BK/book_deep2.bin JOBS=12 bash /mnt/c/regicide-match/jobs/r4/build_win_variant.sh > $J2/build_deep2.txt 2>&1
B=~/regicide-match/builds/6bb8c6c-deep2-egdb8
{ echo "-- linux bench with set book off"; printf 'set book off\nbench\nquit\n' | $B/build-linux/regicide 2>&1 | grep -E '^bench'
  echo "-- cbhost"; printf 'cmd get dbpieces\ncmd get book\ncmd set avoidfile builtin\nquit\n' | \
    /mnt/c/regicide-match/bin/b71dfa9/cbhost.exe 'C:\regicide-match\engines\regicide\6bb8c6c-deep2-egdb8\regicide_cb.dll' 2>&1 | tr -d '\r'; } >> $J2/build_deep2.txt
st "DLL built: $(grep -E '^bench|^reply' $J2/build_deep2.txt | tr '\n' ' ')"
# 5. tests
export OPENINGS=$J2/target_openings.txt
for i in 1 2 3; do
  bash $R 6bb8c6c-deep2-egdb8 d2t$i cake 0.5
  bash $R 6bb8c6c-egdb8 d1t$i cake 0.5
done
st "TARGETED_DONE"
unset OPENINGS
bash $R 6bb8c6c-deep2-egdb8 deep2-full cake 0.5
bash $R 6bb8c6c-deep2-egdb8 deep2-full kingsrow 0.5
st "D2_ALL_DONE"
