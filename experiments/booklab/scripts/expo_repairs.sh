#!/usr/bin/env bash
# How many games of a targeted test met each WP13 repair decision (reached the position with Regicide to move and
# played the repair move), per opponent.   expo_repairs.sh BOOK TESTTAG   (e.g. book_r11c.bin w13-t7-r11c)
L=~/regicide-jobs/booklab; R=~/regicide-match/results; W=~/regicide-booklab
BOOK=$1; TAG=$2
first() { grep -v '^#' $1 | grep ' | ' | head -1 | awk -F' [|] ' '{print $1 "\t" $2}'; }
{
  printf '#60 ply 11\t'; first $L/wp13/rep_cake/ACF_60_Black_Hole_B_kingsrow.txt
  printf '#71 ply 13\t'; first $L/wp13/rep_cake/ACF_71_Diabolical_Denny_B_kingsrow.txt
  printf '#141/#9 ply 7\t'; first $L/wp13/rep_cake/ACF_141_B_kingsrow.txt
  printf '#34 ply 19\t'; first $L/wp13/rep_t2/ACF_34_Twilight_Zone_B_kingsrow_t2.txt
  printf '#62 ply 17\t%s\t4-8\n' "$(cat $L/wp13/g62_fen.txt)"
  printf '#93 ply 19\t%s\t2-7\n' "$(awk -F'\t' '$1=="g93:19"{print $2}' $L/wp13/g93_fens.tsv)"
  printf '#53 ply 6\t%s\t22-17\n' "$(cut -f2 $L/wp13/g53_fens.tsv)"
} > /tmp/repairs.tsv
python3 - "$BOOK" "$TAG" <<'PY'
import sys, subprocess
sys.path.insert(0, '/home/admingod/regicide-booklab/tools/booklab')
from theory import parse_pdn
L = '/home/admingod/regicide-jobs/booklab'; R = '/home/admingod/regicide-match/results'
book, tag = sys.argv[1], sys.argv[2]
reps = [l.rstrip('\n').split('\t') for l in open('/tmp/repairs.tsv')]
print('| Repair | ' + ' | '.join(f'{o}: met / games of the line' for o in ('kingsrow', 'cake', 'guinn')) + ' |')
print('|---|---|---|---|')
res = {r[0]: [] for r in reps}
for opp in ('kingsrow', 'cake', 'guinn'):
    games = list(parse_pdn(open(f'{R}/bl-{tag}-{opp}/games.pdn').read()))
    lines = ''.join(f"{i}\t\t{'B' if t['Black'].lower().startswith('regicide') else 'W'}\t3\t{' '.join(x for x, _ in m)}\n" for i, (t, m) in enumerate(games))
    out = subprocess.run(['/home/admingod/regicide-booklab/build-linux/booklab_pinwalk', f'{L}/{book}', '/home/admingod/regicide-data/egdb'],
                         input=lines, capture_output=True, text=True).stdout.splitlines()
    for name, fen, mv in reps:
        hit = {r.split('\t')[0] for r in out if r.split('\t')[2] == fen and r.split('\t')[3] == mv}
        line = {'#60': '10-14 22-17 9-13', '#71': '10-14 23-19 7-10', '#141/#9': None, '#34': '9-13 24-20 11-16',
                '#62': '10-14 22-17 11-16', '#93': '10-15 22-17 9-13', '#53': '9-14 24-19 11-15'}[name.split()[0]]
        n = sum(1 for t, m in games if (line is None and ' '.join(x for x, _ in m[:3]) in ('11-16 22-17 9-13', '9-13 22-18 11-16'))
                or (line is not None and ' '.join(x for x, _ in m[:3]) == line))
        res[name].append(f"{len(hit)} / {n}")
for name, v in res.items():
    print(f"| {name} | " + ' | '.join(v) + ' |')
PY
