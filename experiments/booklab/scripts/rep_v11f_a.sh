R=~/regicide-match/results; S=/mnt/c/regicide-match/jobs/booklab/src/tools/booklab; L=~/regicide-jobs/booklab
for f in "" "--searched-only"; do
python3 $S/wp14_line_report.py $f --opening "ACF #6 Inferno" --colour B --md $L/wp11f/v11f_a${f:+_searched}.md --group \
  "r11f fresh=$R/bl-v11f-c6-fresh" "r11f kept=$R/bl-v11f-c6-kept" "r11f natural (Black games)=$R/bl-v11f-c6-natural"
done
# book coverage: own moves by book (depth 0/0, nodes 0) and the first searched ply per game
python3 - <<'PY'
import sys, os, re, collections
sys.path.insert(0, '/mnt/c/regicide-match/jobs/booklab/src/tools/booklab')
from theory import parse_pdn
R = os.path.expanduser('~/regicide-match/results'); c = collections.Counter(); first = []
for run in ('bl-v11f-c6-fresh', 'bl-v11f-c6-kept', 'bl-v11f-c6-natural'):
    for t, m in parse_pdn(open(f'{R}/{run}/games.pdn').read()):
        if not t['Black'].lower().startswith('regicide'): continue
        c[(run, m[16][0])] += 1
        fs = next((i + 1 for i in range(3, len(m), 2) if not re.search(r'depth 0/0.*nodes 0', m[i - 1][1] or '') and i - 1 >= 4 and (i - 1) % 2 == 0), None)
        fs = next((i + 1 for i, (mv, cm) in enumerate(m) if i % 2 == 0 and i >= 4 and not re.search(r'depth 0/0.*nodes 0', cm or '')), None)
        first.append(fs)
print('ply 17 by run:', dict(c))
fs = sorted(x for x in first if x); print('first own searched ply: min', fs[0], 'median', fs[len(fs)//2], 'max', fs[-1])
PY
