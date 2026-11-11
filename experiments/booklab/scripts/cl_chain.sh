#!/usr/bin/env bash
# (a) Claims census (lead 2026-11-11 11:57): master 8de39d0 as committed (DLL bl-8de39d0: built-in book r11d, net n8q;
# runner bin/match-8de3, host bin/8de3/cbhost.exe; claims recorded in every PDN: FirstClaim, ClaimsA/B, claim= in the
# comments), one production deck against each opponent (KingsRow mode 3, Cake, GuiNN), concurrency 8,
# --pairs-together, processes kept, NO --cb-claims. Tags cl-<opp>-<half>.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
st() { echo "$* $(date -Is)" >> $L/status; }
export MATCH=$L/bin/match-8de3 CBHOST=$L/bin/8de3/cbhost.exe
st "CL start (runner $(sha256sum $MATCH | cut -c1-16), DLL $(sha256sum /mnt/c/regicide-match/engines/regicide/bl-8de39d0/regicide_cb.dll | cut -c1-16))"
for opp in kingsrow cake guinn; do
  for h in h1 h2; do
    EXTRA_ARGS="--allow-rejected-options --pairs-together" bash $J/bl_run_w.sh regicide:bl-8de39d0 $opp cl-$opp-$h $J/acf156_$h.txt 0.5 8 < /dev/null \
      || { st "CL STOPPED at cl-$opp-$h"; exit 1; }
  done
done
st "CL done"
