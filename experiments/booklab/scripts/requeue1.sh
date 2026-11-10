#!/usr/bin/env bash
# re-queue runs whose lock waiter disappeared at about 08:20 (no start/end in the status log)
J=/mnt/c/regicide-match/jobs/booklab
echo "REQUEUE v2-cake-b-h1 and t6-r6-guinn $(date -Is)" >> ~/regicide-jobs/booklab/status
bash $J/bl_run.sh regicide:bl-r5 cake v2-cake-b-h1 $J/acf156_h1.txt < /dev/null
bash $J/bl_target.sh bl-r6 t6-r6-guinn guinn 2 "10-15 22-17 15-19"
sleep 3
