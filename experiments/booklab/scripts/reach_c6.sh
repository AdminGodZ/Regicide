L=~/regicide-jobs/booklab; W=~/regicide-booklab; J=/mnt/c/regicide-match/jobs/booklab
FEN="B:W19,21,22,23,27,28,30,31,32:B2,4,5,6,7,8,12,13,14"
python3 - "$FEN" > $L/wp14_reach_q.tsv <<'PY'
import sys
sys.path.insert(0, '/mnt/c/regicide-match/jobs/booklab/src/tools/booklab')
from theory import acf_name
for l in open('/mnt/c/regicide-match/jobs/booklab/acf156_all.txt'):
    mv = l.strip().strip('|').split('|')[0].strip()
    if mv: print(f"{acf_name(mv)}\t{mv}\t{sys.argv[1]}\t30")
PY
wc -l < $L/wp14_reach_q.tsv
nice $W/build-linux/booklab_reach $L/book_r11d.bin 3000000 ~/regicide-data/egdb < $L/wp14_reach_q.tsv > $L/wp14_reach.out
cut -f2 $L/wp14_reach.out | sort | uniq -c
awk -F'\t' '$2 != "unreachable"' $L/wp14_reach.out | cut -c1-200
# recorded games: every game whose position with Black to move equals FEN, by opening
python3 - "$FEN" <<'PY'
import sys, glob, os, collections, subprocess
sys.path.insert(0, '/mnt/c/regicide-match/jobs/booklab/src/tools/booklab')
from theory import parse_pdn, ballot_of
c = collections.Counter(); inp = []; meta = []
for d in glob.glob(os.path.expanduser('~/regicide-match/results/bl-*')):
    p = d + '/games.pdn'
    if not os.path.exists(p) or '.' in os.path.basename(d): continue
    for t, m in parse_pdn(open(p, errors='replace').read()):
        if not t['Black'].lower().startswith('regicide'): continue
        inp.append(f"{len(meta)}\t{t.get('FEN','')}\tB\t0\t{' '.join(x for x, _ in m[:30])}\n"); meta.append(ballot_of(t['Opening'])[0])
out = subprocess.run([os.path.expanduser('~/regicide-jobs/booklab/bin/blk-pinwalk-1c6e'), os.path.expanduser('~/regicide-jobs/booklab/book_r11d.bin')], input=''.join(inp), capture_output=True, text=True).stdout
for l in out.splitlines():
    f = l.split('\t')
    if len(f) > 2 and f[2] == sys.argv[1]: c[(meta[int(f[0])], int(f[1]) + 1)] += 1
print('recorded games reaching the FEN (opening, ply):', dict(c))
PY
