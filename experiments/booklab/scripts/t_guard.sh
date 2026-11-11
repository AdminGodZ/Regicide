# Test of the wp12_chain completeness guard (review 2026-11-11): exit_status 0 with 1 of 156 games must be refused.
T=$(mktemp -d); st() { echo "ST: $*"; }
mk() { mkdir -p $T/$1; [ -n "$2" ] && echo $2 > $T/$1/exit_status; echo "Games: $3 of 156 scheduled" > $T/$1/summary.txt; }
mk ok 0 156; mk partial 0 1; mk old "" 156; mk oldpartial "" 1; mk failed 4 156
for d in $T/ok $T/partial $T/old $T/oldpartial $T/failed; do
  ( { [ ! -f $d/exit_status ] || [ "$(cat $d/exit_status)" = 0 ]; } && grep -Eq "^Games: ([0-9]+) of \1 scheduled" $d/summary.txt \
    || { st "W12 not started: $d not complete"; exit 1; } ) > /dev/null && echo "$(basename $d): accepted" || echo "$(basename $d): refused"
done
rm -rf $T
grep -q 'grep -Eq "^Games: (\[0-9\]+) of \1 scheduled" $d/summary.txt' /mnt/c/regicide-match/jobs/booklab/wp12_chain.sh && echo "guard text in wp12_chain.sh matches"
python3 - <<'PY'
import ast
src = open('/mnt/c/regicide-match/jobs/booklab/src/tools/booklab/wp12_confirm.py').read()
i, j = src.index('fcntl.flock(lk, fcntl.LOCK_EX)'), src.index('readback(regdll(a.cand_dll)')
print('wp12_confirm: lock before read-back' if i < j else 'WRONG ORDER')
src = open('/mnt/c/regicide-match/jobs/booklab/src/tools/booklab/wp12_discover.py').read()
i, j = src.index('fcntl.flock(lk, fcntl.LOCK_EX)'), src.index('ok_a, ra = readback(')
print('wp12_discover: lock before read-back' if i < j else 'WRONG ORDER')
for f in ('wp12_confirm.py', 'wp12_discover.py'): ast.parse(open('/mnt/c/regicide-match/jobs/booklab/src/tools/booklab/' + f).read())
PY
bash -n /mnt/c/regicide-match/jobs/booklab/wp12_chain.sh && echo "wp12_chain.sh parses"
