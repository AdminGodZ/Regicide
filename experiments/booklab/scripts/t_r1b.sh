#!/usr/bin/env bash
J=/mnt/c/regicide-match/jobs/booklab
B=("11-16 23-19 16x23" "9-13 22-18 10-15" "10-14 22-17 11-16" "10-14 24-19 14-18" "10-15 21-17 15-18")
bash $J/bl_target.sh bl-r1 t1-r1-kingsrow kingsrow 4 "${B[@]}"
bash $J/bl_target.sh bl-r1 t1-r1-guinn guinn 4 "${B[@]}"
sleep 3
