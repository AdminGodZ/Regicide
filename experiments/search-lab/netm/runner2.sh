#!/bin/bash
# Net-match runner v2. queue.txt lines: tag|netA|netB ("builtin" = the engine's built-in n6c)|engine dir|movetime
# A match = openings4 in 6 chunks (chunks/op4_a..f, a partition: check_chunks.py), one tools/match per chunk with
# -c 1, pinned to one physical P-core (logical CPUs 2k, 2k+1). A match counts as done only when validate.py --pooled
# passes: every chunk "Games: N of N" (not interrupted) with N = 2 x its openings and N rows in results.csv, every
# worker and the pooling step exited 0, and the pool has 805 pairs (validate.py: one game per colour per planned pair,
# distinct game ids, the planned opening, score consistent with result and colours). Incomplete chunks are deleted and re-run (at most
# 3 attempts per match per runner start); only then is the line written to done.txt.
N=~/regicide-jobs/netm; REPO=~/Documents/Projects/Regicide
echo "RUNNER2 START $(date -Is)"
python3 -I $N/check_chunks.py >/dev/null || { echo "chunk files are not a partition of openings4: abort"; exit 1; }
fetch_net() {
  [ -s $N/nets/$1.nnue ] && return 0
  git -C $REPO fetch -q origin t3/truth-data 2>/dev/null
  git -C $REPO show origin/t3/truth-data:experiments/truth-data/$1.nnue > $N/nets/$1.nnue.tmp 2>/dev/null \
    && mv $N/nets/$1.nnue.tmp $N/nets/$1.nnue && echo "FETCHED $1 $(date -Is)" && return 0
  rm -f $N/nets/$1.nnue.tmp; return 1
}
run_chunk() {  # run_chunk OUT LETTER INDEX NA NB ENG MT  (records the exit status in OUT/LETTER.rc)
  local out=$1 c=$2 k=$3 na=$4 nb=$5 eng=$6 mt=$7
  local optb=(--option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0")
  [ "$nb" = builtin ] || optb+=(--option-b "set evalfile $N/nets/$nb.nnue")
  rm -rf $out/$c $out/$c.rc
  taskset -c $((2*k)),$((2*k+1)) $N/bin/match --cbhost $N/bin/cbhost -a $N/$eng/libregicide_cb.so -b $N/$eng/libregicide_cb.so \
    --name-a $na --name-b $nb --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" \
    --option-a "set evalfile $N/nets/$na.nnue" "${optb[@]}" --openings $N/chunks/op4_$c --movetime $mt -c 1 \
    --pdn-comments -q -o $out/$c > $out/$c.log 2>&1
  echo $? > $out/$c.rc
}
while [ ! -f $N/STOP ]; do
  line=""
  while IFS= read -r l; do
    [ -z "$l" ] || [[ "$l" == \#* ]] && continue
    /usr/bin/grep -qxF "$l" $N/failed.txt 2>/dev/null && continue
    IFS='|' read -r tag na nb eng mt <<<"$l"
    if python3 -I $N/validate.py $N/matches/$tag --pooled > /dev/null 2>&1; then
      /usr/bin/grep -qxF "$l" $N/done.txt || echo "$l" >> $N/done.txt
      continue
    fi
    ok=1; fetch_net $na || ok=0; [ "$nb" = builtin ] || fetch_net $nb || ok=0
    [ $ok = 1 ] && { line="$l"; break; }
  done < $N/queue.txt
  if [ -z "$line" ]; then echo "IDLE $(date -Is)"; sleep 300; continue; fi
  IFS='|' read -r tag na nb eng mt <<<"$line"; out=$N/matches/$tag; mkdir -p $out
  s=$(date +%s); complete=0
  for attempt in 1 2 3; do
    missing=$(python3 -I $N/validate.py $out | tail -1 | sed -n 's/^INCOMPLETE //p')
    echo "BEGIN $tag attempt $attempt chunks '${missing}' A=$na B=$nb eng=$eng mt=$mt $(date -Is)"
    k=0
    for c in a b c d e f; do
      [[ "$missing" == *$c* ]] && run_chunk $out $c $k $na $nb $eng $mt &
      k=$((k+1))
    done
    wait
    # strict pool: only pairs validate.py accepts (pool_dirs.py pooled any two rows with the same pair number)
    python3 -I $N/validate.py $out --write-pool $out/pooled.txt > /dev/null 2>&1; echo 0 > $out/pool.rc
    python3 -I $N/stats.py $na $out/? > $out/stats.txt 2>&1
    if python3 -I $N/validate.py $out --pooled > $out/validate.txt 2>&1; then complete=1; break; fi
    echo "INCOMPLETE $tag after attempt $attempt: $(tr '\n' ' ' < $out/validate.txt)"
    [ -f $N/STOP ] && break
  done
  if [ $complete = 1 ]; then
    echo "DONE $tag secs=$(( $(date +%s)-s )) $(date -Is) :: $(tail -1 $out/pooled.txt)"
    echo "$line" >> $N/done.txt
  else
    echo "FAILED $tag $(date -Is)"; [ -f $N/STOP ] || echo "$line" >> $N/failed.txt
  fi
done
echo "STOPPED $(date -Is)"
