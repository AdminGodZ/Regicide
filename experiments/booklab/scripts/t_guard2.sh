#!/usr/bin/env bash
# Test of the WP12 context guard (run_complete.sh, called by wp12_chain.sh): made-up run directories, expected
# decisions asserted; exit 1 on any wrong decision. Also checks that wp12_chain.sh calls run_complete.sh.
J=/mnt/c/regicide-match/jobs/booklab
T=$(mktemp -d); fail=0
mk() { mkdir -p $T/$1; [ -n "$2" ] && echo $2 > $T/$1/exit_status; [ -n "$3" ] && echo "Games: $3 of 156 scheduled" > $T/$1/summary.txt; return 0; }
mk ok 0 156; mk partial 0 1; mk old "" 156; mk oldpartial "" 1; mk failed 4 156; mk nosummary 0 ""
for c in "ok accept" "partial refuse" "old accept" "oldpartial refuse" "failed refuse" "nosummary refuse"; do
  set -- $c
  bash $J/run_complete.sh $T/$1 && got=accept || got=refuse
  [ $got = $2 ] && echo "ok   $1: $got" || { echo "FAIL $1: $got, expected $2"; fail=1; }
done
rm -rf $T
grep -q 'bash $J/run_complete.sh $d ||' $J/wp12_chain.sh && echo "ok   wp12_chain.sh uses run_complete.sh" || { echo "FAIL wp12_chain.sh does not call run_complete.sh"; fail=1; }
[ $fail = 0 ] && echo "PASS" || echo "FAILED"
exit $fail
