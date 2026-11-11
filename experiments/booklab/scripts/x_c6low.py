import sys, os, re, glob, collections
sys.path.insert(0, '/mnt/c/regicide-match/jobs/booklab/src/tools/booklab')
from theory import parse_pdn
R = os.path.expanduser('~/regicide-match/results')
lows = collections.Counter(); tasks = {}
for run in ('bl-w14c6-kingsrow-fresh', 'bl-w14c6-kingsrow-kept'):
    for t, m in parse_pdn(open(f'{R}/{run}/games.pdn').read()):
        best = None
        for i, (mv, c) in enumerate(m):
            if i % 2 == 0:
                s = re.search(r'score (-?\d+)', c or '')
                if s and (best is None or int(s.group(1)) < best[0]): best = (int(s.group(1)), i)
        v, i = best
        lows[(run.split('-')[-1], v, i + 1, m[i][0])] += 1
        key = ' '.join(x for x, _ in m[:i])
        tasks.setdefault(key, f"low{len(tasks)+1}:{run.split('-')[-1]}:{i+1}:{v}")
for k, n in sorted(lows.items()): print(k, n)
with open('/mnt/c/regicide-match/jobs/booklab/wp14/c6low.tsv', 'w') as fo:
    for key, tid in list(tasks.items())[:6]:
        fo.write(f"{tid}\t\t{key}\t\n")
print(len(tasks), 'distinct low-point positions; first 6 written')
