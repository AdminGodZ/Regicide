#!/usr/bin/env bash
# WP1 census: each pairing as two halves of acf156 (78 ballots x 2 colours each, about 14 min per half, one timing-lock
# hold per half). Cake-Cake first (its mirror games are what WP5 needs first). Run in an attached wsl.exe.
J=/mnt/c/regicide-match/jobs/booklab
R=regicide:6bb8c6c-deep2b-egdb8
H1=$J/acf156_h1.txt; H2=$J/acf156_h2.txt
L=~/regicide-jobs/booklab
echo "CENSUS start $(date -Is)" >> $L/status
while read -r a b tag only; do
  [ -z "$a" ] && continue
  bash $J/bl_run.sh "$a" "$b" "$tag-h1" $H1 < /dev/null
  [ -z "$only" ] && bash $J/bl_run.sh "$a" "$b" "$tag-h2" $H2 < /dev/null
done <<EOF
cake cake2 c-cake-cake
$R guinn c-reg-guinn
kingsrow kingsrow2 c-kingsrow-kingsrow
guinn guinn2 c-guinn-guinn
cake kingsrow c-cake-kingsrow
cake guinn c-cake-guinn
kingsrow guinn c-kingsrow-guinn
cake cake2 c-cake-cake-rep ONLYH1
$R guinn c-reg-guinn-rep ONLYH1
EOF
echo "CENSUS done $(date -Is)" >> $L/status
sleep 3
