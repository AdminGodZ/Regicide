# Usage: runmatches.sh "A:B" ["A:B" ...]   (default: the four planned pairings)
W=~/regicide-evalab-work
M=~/regicide-evalab/build-G0/tools/match
ST=$W/matches/status
mkdir -p $W/matches
baseline_busy() { pgrep -f "regicide-match/bin/match" > /dev/null || pgrep -f "regicide-jobs/match/run_baseline" > /dev/null; }
PAIRS=("$@"); [ ${#PAIRS[@]} -eq 0 ] && PAIRS=(V1:G0 B:V1 K:V1 BK:V1)
for p in "${PAIRS[@]}"; do
  A=${p%%:*}; B=${p##*:}; OUT=$W/matches/$A-vs-$B
  if [ -f $OUT/summary.txt ] && grep -q "Games: 1610 of 1610" $OUT/summary.txt; then echo "skip $p" >> $ST; continue; fi
  while baseline_busy; do sleep 60; done
  echo "start $p $(date -Is)" >> $ST
  rm -rf $OUT
  $M -a $W/engines/$A.so -b $W/engines/$B.so --name-a $A --name-b $B \
    --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" \
    --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" \
    --openings $W/openings4.txt --movetime 0.1 -c 10 --pdn-comments -o $OUT > $W/matches/$A-vs-$B.log 2>&1
  echo "end $p rc=$? $(date -Is)" >> $ST
done
echo "ALL_DONE $(date -Is)" >> $ST
