#!/usr/bin/env bash
# Run inside an attached wsl.exe (Start-Process hidden). Deep targeted opening book (master ff0a3d2, built-in n6c), after
# the TM series and the T6 confirmation runs:
#  1. roots.txt: the position after the 3-move ballot of every ballot Regicide lost (as Black or White) today (deep_roots.py)
#  2. calibration: bookgen --roots roots.txt --nodes 30000000 --expansions 60 --threads 20 (timestamped output)
#  3. main: bookgen --resume book_deep.bin --roots roots.txt ... --expansions N, N chosen so the whole book takes ~6.5 h
#  4. DLL ff0a3d2-deep-egdb8 with book_deep.bin embedded (default CB book mode 3 = best moves), 2-8 EGDB
#  5. Cake 312 games at 0.5 s: book off (same DLL, "set book 0") and book on; KingsRow and GuiNN with the book on.
# gen5 yields to bookgen (gen5yield, pausekeeper5); during the CheckerBoard runs gen5a runs alongside (12 threads).
J=~/regicide-jobs/r4; JD=~/regicide-jobs/deep; BG=$JD/bin/bookgen-ff0a3d2; BK=~/regicide-data/book/book_deep.bin
R=/mnt/c/regicide-match/jobs/ba/run_ba2.sh
st() { echo "DEEP $* $(date -Is)" >> $J/status; }
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
ts() { while IFS= read -r l; do printf '%s %s\n' "$(date +%s)" "$l"; done; }
until grep -q '^TM T6CONF_DONE' $J/status; do sleep 30; done
while [ -e $J/matchlock ] || [ -e $J/baselock ]; do sleep 20; done
touch $J/gen5yield
# ff0a3d2: plain Windows DLL (2-8 egdb) + Linux tree; bench 8,089,571 expected; bookgen from the same tree
if [ ! -x $BG ]; then
  SHA=ff0a3d2 OUT=ff0a3d2-egdb8 JOBS=12 bash /mnt/c/regicide-match/jobs/r4/build_win_variant.sh > $JD/build_ff0a3d2.txt 2>&1
  BL=~/regicide-match/builds/ff0a3d2-egdb8/build-linux
  nice cmake --build $BL -j 12 >> $JD/build_ff0a3d2.txt 2>&1
  { echo "-- linux ctest"; ctest --test-dir $BL -j 8 2>&1 | tail -n 3; } >> $JD/build_ff0a3d2.txt
  mkdir -p $JD/bin; cp $BL/bookgen $BG
  st "ff0a3d2 build: $(grep -E '^bench|tests passed' $JD/build_ff0a3d2.txt | tr '\n' ' ')"
fi
python3 /mnt/c/regicide-match/jobs/deep/deep_roots.py > $JD/roots.out 2>&1
st "roots: $(head -n 1 $JD/roots.out); $(sed -n 2p $JD/roots.out)"
TOTAL=$(( 390 * 60 ))      # target running time of the whole book, s
if [ ! -e $BK ]; then
  T0=$(date +%s)
  st "calibration start: bookgen-ff0a3d2 --roots roots.txt --nodes 30000000 --expansions 60 --threads 20"
  stdbuf -oL $BG --roots $JD/roots.txt --nodes 30000000 --expansions 60 --threads 20 --out $BK 2>&1 | ts > $JD/calib.log
  T1=$(date +%s)
  # rate from the expansion lines after the roots (first "expanded" line to the last)
  read E0 S0 <<< "$(grep ' expanded ' $JD/calib.log | head -n 1 | awk '{print $3, $1}')"
  read E1 S1 <<< "$(grep ' expanded ' $JD/calib.log | tail -n 1 | awk '{print $3, $1}')"
  RATE=$(awk "BEGIN{d=$S1-$S0; print (d>0 && $E1>$E0) ? ($E1-$E0)/d : 0}")
  [ "$RATE" = 0 ] && RATE=$(awk "BEGIN{print 60/($T1-$T0)}")
  N=$(awk "BEGIN{n=int($RATE*($TOTAL-($T1-$T0))); if (n<200) n=200; print n}")
  st "calibration done in $((T1-T0)) s: $(grep -E 'roots analysed|wrote' $JD/calib.log | cut -d' ' -f2- | tr '\n' ' '); rate $RATE expansions/s -> main run --expansions $N (~$(( (TOTAL-(T1-T0))/60 )) min)"
  echo $N > $JD/main_expansions
fi
if ! grep -q '^DEEP main end rc=0' $J/status; then
  N=$(cat $JD/main_expansions)
  cp $BK $BK.calib
  st "main start: bookgen-ff0a3d2 --resume book_deep.bin --roots roots.txt --nodes 30000000 --expansions $N --threads 20"
  stdbuf -oL $BG --resume $BK --roots $JD/roots.txt --nodes 30000000 --expansions $N --threads 20 --out $BK 2>&1 | ts > $JD/main.log
  st "main end rc=${PIPESTATUS[0]} $(tail -n 1 $JD/main.log | cut -d' ' -f2-)"
fi
SHA=ff0a3d2 OUT=ff0a3d2-deep-egdb8 BOOKFILE=$BK JOBS=6 bash /mnt/c/regicide-match/jobs/r4/build_win_variant.sh > $JD/build_deep.txt 2>&1
B=~/regicide-match/builds/ff0a3d2-deep-egdb8
{ echo "-- linux bench with set book off"; printf 'set book off\nbench\nquit\n' | $B/build-linux/regicide 2>&1 | grep -E '^bench'
  echo "-- cbhost: dbpieces, book mode, start position"
  printf 'cmd get dbpieces\ncmd get book\nposition startpos moves 9-13 22-18 10-14\ngo maxtime 0.5 reset\nquit\n' | \
    /mnt/c/regicide-match/bin/b71dfa9/cbhost.exe 'C:\regicide-match\engines\regicide\ff0a3d2-deep-egdb8\regicide_cb.dll' 2>&1 | tr -d '\r'; } >> $JD/build_deep.txt
st "DLL built: $(grep -E '^bench' $JD/build_deep.txt | tr '\n' ' ')"
bash $R ff0a3d2-deep-egdb8 deep-off cake 0.5 "set book 0"
bash $R ff0a3d2-deep-egdb8 deep-on cake 0.5
bash $R ff0a3d2-deep-egdb8 deep-on kingsrow 0.5
bash $R ff0a3d2-deep-egdb8 deep-on guinn 0.5
st "DEEP_ALL_DONE"
