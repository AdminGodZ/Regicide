R=~/regicide-match/results; S=/mnt/c/regicide-match/jobs/booklab/src/tools/booklab; O=/mnt/c/regicide-match/jobs/booklab/wp14
j() { ls -d "$@" 2>/dev/null | grep -v '\.' | tr '\n' ',' | sed 's/,$//'; }
grep -E "BL end w14c6-kingsrow" ~/regicide-jobs/booklab/status | cut -c1-110
python3 $S/wp14_line_report.py --opening "ACF #6 Inferno" --colour B --md $O/c6_table.md --group \
  "new 8-11 KingsRow fresh=$R/bl-w14c6-kingsrow-fresh" "new 8-11 KingsRow kept=$R/bl-w14c6-kingsrow-kept" \
  "new 8-11 Cake fresh=$R/bl-w14c6-cake-fresh" "new 8-11 Cake kept=$R/bl-w14c6-cake-kept" \
  "new 8-11 GuiNN fresh=$R/bl-w14c6-guinn-fresh" "new 8-11 GuiNN kept=$R/bl-w14c6-guinn-kept" \
  "old 6-10 KingsRow (records)=$(j $R/bl-mo-kingsrow-* $R/bl-mo1-kingsrow $R/bl-mo2-kingsrow $R/bl-cl-kingsrow-* $R/bl-v11d-n8q-kingsrow-*)" \
  "old 6-10 Cake (records)=$(j $R/bl-mo-cake-* $R/bl-mo1-cake $R/bl-mo2-cake $R/bl-cl-cake-* $R/bl-v11d-n8q-cake-* $R/bl-e2c-*)" \
  "old 6-10 GuiNN (records)=$(j $R/bl-mo-guinn-* $R/bl-mo1-guinn $R/bl-mo2-guinn $R/bl-cl-guinn-* $R/bl-v11d-n8q-guinn-*)"
# did every new game play 8-11 at ply 17 from the book?
python3 - <<'PY'
import sys, glob, os, re, collections
sys.path.insert(0, '/mnt/c/regicide-match/jobs/booklab/src/tools/booklab')
from theory import parse_pdn
c = collections.Counter()
for d in glob.glob(os.path.expanduser('~/regicide-match/results/bl-w14c6-*')):
    for t, m in parse_pdn(open(d + '/games.pdn').read()):
        mv = [x for x, _ in m]
        c[(os.path.basename(d), mv[16] if len(mv) > 16 else '-', 'book' if re.search(r'depth 0/0.*nodes 0', m[16][1] or '') else 'search', ' '.join(mv[:16]) == '9-13 22-18 10-14 18x9 5x14 25-22 11-15 22-18 15x22 26x17x10 7x14 29-25 3-7 25-22 1-5 24-19')] += 1
for k, n in sorted(c.items()): print('ply 17:', k, n)
PY
