#!/usr/bin/env bash
# WP13 validation of r10k (DLL bl-456e9b2-r10k2 = book r10k, try 2): two production-deck runs against KingsRow in book
# mode 3 and one each against Cake and GuiNN, final-validation runner (match-747ab51), everything as the final
# validation (set book 3 read back for every engine). Tags v13-<opp>-<run>-<half>.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
st() { echo "$* $(date -Is)" >> $L/status; }
st "WP13 validation start (r10k)"
for spec in "kingsrow a" "kingsrow b" "cake a" "guinn a"; do
  set -- $spec
  for h in h1 h2; do
    bash $J/bl_run_x.sh regicide:bl-456e9b2-r10k2 $1 v13-$1-$2-$h $J/acf156_$h.txt < /dev/null \
      || { st "WP13 validation STOPPED at v13-$1-$2-$h"; exit 1; }
  done
done
st "WP13 validation done"
