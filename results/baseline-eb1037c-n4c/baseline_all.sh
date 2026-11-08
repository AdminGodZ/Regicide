#!/usr/bin/env bash
# Run inside an attached wsl.exe (Start-Process hidden): baseline_all.sh NAME
# 0.5 s baseline vs KingsRow, Cake, GuiNN, then the 2 s/move KingsRow run.
export NAME=$1
RUNS="kingsrow cake guinn" bash /mnt/c/regicide-match/jobs/r4/run_baseline_r4.sh
RUNS="kingsrow-2s" bash /mnt/c/regicide-match/jobs/r4/run_baseline_r4.sh
echo "BASELINE_WRAPPER_DONE $NAME $(date -Is)" >> ~/regicide-jobs/r4/status
