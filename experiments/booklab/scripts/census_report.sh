#!/usr/bin/env bash
# census tables -> ~/regicide-jobs/booklab/census_out
C=/mnt/c/regicide-match/jobs/booklab/src/tools/booklab/census.py
REP=()
[ -f ~/regicide-match/results/bl-c-cake-cake-rep-h1/summary.txt ] && REP+=(--repeat "Cake-Cake=bl-c-cake-cake-h1:bl-c-cake-cake-rep-h1")
[ -f ~/regicide-match/results/bl-c-reg-guinn-rep-h1/summary.txt ] && REP+=(--repeat "Regicide-GuiNN=bl-c-reg-guinn-h1:bl-c-reg-guinn-rep-h1")
python3 $C --out ~/regicide-jobs/booklab/census_out "${REP[@]}" \
  "Cake vs KingsRow=bl-c-cake-kingsrow-h1,bl-c-cake-kingsrow-h2" \
  "Cake vs GuiNN=bl-c-cake-guinn-h1,bl-c-cake-guinn-h2" \
  "KingsRow vs GuiNN=bl-c-kingsrow-guinn-h1,bl-c-kingsrow-guinn-h2" \
  "Cake vs Cake=bl-c-cake-cake-h1,bl-c-cake-cake-h2" \
  "KingsRow vs KingsRow=bl-c-kingsrow-kingsrow-h1,bl-c-kingsrow-kingsrow-h2" \
  "GuiNN vs GuiNN=bl-c-guinn-guinn-h1,bl-c-guinn-guinn-h2" \
  "Regicide master vs GuiNN=bl-c-reg-guinn-h1,bl-c-reg-guinn-h2" \
  "(ref) Regicide master vs Cake, ba-deep2b-full=ba-deep2b-full-cake" \
  "(ref) Regicide master vs KingsRow, ba-deep2b-full=ba-deep2b-full-kingsrow"
