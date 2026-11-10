#!/bin/bash
# Net-match runner. queue.txt lines: tag|netA|netB ("builtin" = the engine's built-in n6c)|engine dir|movetime
# Each match = openings4 in 6 chunks (chunks/op4_a..f), one tools/match per chunk with -c 1, each pinned to one physical
# P-core (logical CPUs 2k, 2k+1). A line whose nets are missing is skipped (retried later, after a git fetch).
N=~/regicide-jobs/netm; REPO=~/Documents/Projects/Regicide
echo "RUNNER START $(date -Is)"
fetch_net() {  # fetch_net NAME: copy NAME.nnue from origin/t3/truth-data if it is there
  [ -s $N/nets/$1.nnue ] && return 0
  git -C $REPO fetch -q origin t3/truth-data 2>/dev/null
  git -C $REPO show origin/t3/truth-data:experiments/truth-data/$1.nnue > $N/nets/$1.nnue.tmp 2>/dev/null \
    && mv $N/nets/$1.nnue.tmp $N/nets/$1.nnue && echo "FETCHED $1 $(date -Is)" && return 0
  rm -f $N/nets/$1.nnue.tmp; return 1
}
while [ ! -f $N/STOP ]; do
  line=""
  while IFS= read -r l; do
    [ -z "$l" ] || [[ "$l" == \#* ]] && continue
    grep -qxF "$l" $N/done.txt && continue
    IFS='|' read -r tag na nb eng mt <<<"$l"
    ok=1; fetch_net $na || ok=0; [ "$nb" = builtin ] || fetch_net $nb || ok=0
    [ $ok = 1 ] && { line="$l"; break; }
  done < $N/queue.txt
  if [ -z "$line" ]; then echo "IDLE $(date -Is)"; sleep 300; continue; fi
  IFS='|' read -r tag na nb eng mt <<<"$line"
  echo "BEGIN $tag A=$na B=$nb eng=$eng mt=$mt $(date -Is)"; s=$(date +%s)
  out=$N/matches/$tag; mkdir -p $out
  optb=(--option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0")
  [ "$nb" = builtin ] || optb+=(--option-b "set evalfile $N/nets/$nb.nnue")
  k=0
  for c in a b c d e f; do
    if ! grep -q '^Elo (pent' $out/$c/summary.txt 2>/dev/null; then
      rm -rf $out/$c
      taskset -c $((2*k)),$((2*k+1)) $N/bin/match --cbhost $N/bin/cbhost -a $N/$eng/libregicide_cb.so -b $N/$eng/libregicide_cb.so \
        --name-a $na --name-b $nb --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" \
        --option-a "set evalfile $N/nets/$na.nnue" "${optb[@]}" --openings $N/chunks/op4_$c --movetime $mt -c 1 \
        --pdn-comments -q -o $out/$c > $out/$c.log 2>&1 &
    fi
    k=$((k+1))
  done
  wait
  python3 -I $N/pool_dirs.py $out/? > $out/pooled.txt
  python3 -I $N/stats.py $na $out/? > $out/stats.txt
  echo "DONE $tag secs=$(( $(date +%s)-s )) $(date -Is) :: $(tail -1 $out/pooled.txt)"
  echo "$line" >> $N/done.txt
done
echo "STOPPED $(date -Is)"
