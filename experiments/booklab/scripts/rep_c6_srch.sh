R=~/regicide-match/results; S=/mnt/c/regicide-match/jobs/booklab/src/tools/booklab; O=/mnt/c/regicide-match/jobs/booklab/wp14
j() { ls -d "$@" 2>/dev/null | grep -v '\.' | tr '\n' ',' | sed 's/,$//'; }
python3 $S/wp14_line_report.py --searched-only --opening "ACF #6 Inferno" --colour B --md $O/c6_table_searched.md --group \
  "new 8-11 KingsRow fresh=$R/bl-w14c6-kingsrow-fresh" "new 8-11 KingsRow kept=$R/bl-w14c6-kingsrow-kept" \
  "old 6-10 KingsRow (records)=$(j $R/bl-mo-kingsrow-* $R/bl-mo1-kingsrow $R/bl-mo2-kingsrow $R/bl-cl-kingsrow-* $R/bl-v11d-n8q-kingsrow-*)"
