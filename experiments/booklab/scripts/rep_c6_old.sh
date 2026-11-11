R=~/regicide-match/results; S=/mnt/c/regicide-match/jobs/booklab/src/tools/booklab
j() { ls -d "$@" 2>/dev/null | grep -v '\.' | tr '\n' ',' | sed 's/,$//'; }
python3 $S/wp14_line_report.py --opening "ACF #6 Inferno" --colour B --group \
  "old-kingsrow=$(j $R/bl-mo-kingsrow-* $R/bl-mo1-kingsrow $R/bl-mo2-kingsrow $R/bl-cl-kingsrow-* $R/bl-v11d-n8q-kingsrow-*)" \
  "old-cake=$(j $R/bl-mo-cake-* $R/bl-mo1-cake $R/bl-mo2-cake $R/bl-cl-cake-* $R/bl-v11d-n8q-cake-* $R/bl-e2c-*)" \
  "old-guinn=$(j $R/bl-mo-guinn-* $R/bl-mo1-guinn $R/bl-mo2-guinn $R/bl-cl-guinn-* $R/bl-v11d-n8q-guinn-*)"
