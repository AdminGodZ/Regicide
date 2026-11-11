L=~/regicide-jobs/booklab
echo "my processes:"; ps -eo pid,etime,args | grep -E "booklab|bl_run_w|match-|wp12_|blk-|session.sh|cbhost|wait.sh|rootscore" | grep -v -E "grep|final_check" | cut -c1-120
echo "timing.lock holders:"; (command -v fuser > /dev/null && fuser ~/regicide-jobs/locks/timing.lock 2>&1) || lsof ~/regicide-jobs/locks/timing.lock 2>/dev/null || echo "(none found)"
for f in $L/nolock-*.lock; do [ -f "$f" ] && { lsof "$f" > /dev/null 2>&1 && echo "held: $f" || rm -v "$f"; }; done
