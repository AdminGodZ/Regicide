#!/usr/bin/env bash
J=/mnt/c/regicide-match/jobs/booklab
B=("10-14 22-17 7-10" "10-14 22-17 14-18")
bash $J/bl_target.sh bl-r2 t2-r2-guinn guinn 10 "${B[@]}"
bash $J/bl_target.sh bl-r2 t2-r2-cake cake 4 "${B[@]}"
bash $J/bl_target.sh bl-r2 t2-r2-kingsrow kingsrow 4 "${B[@]}"
sleep 3
