#!/usr/bin/env bash
# r3 = r2 + 7 sweep forces: 4 games per colour on each sweep ballot vs each engine
J=/mnt/c/regicide-match/jobs/booklab
B=("9-13 22-18 6-9" "9-13 23-18 6-9" "9-13 24-20 11-16" "10-14 24-19 7-10" "11-15 24-20 15-18" "12-16 24-20 10-15" "10-14 22-17 11-16")
bash $J/bl_target.sh bl-r3 t3-r3-cake cake 4 "${B[@]}"
bash $J/bl_target.sh bl-r3 t3-r3-kingsrow kingsrow 4 "${B[@]}"
bash $J/bl_target.sh bl-r3 t3-r3-guinn guinn 4 "${B[@]}"
sleep 3
