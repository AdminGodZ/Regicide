import sys, glob, os, re
sys.path.insert(0, '/mnt/c/regicide-match/jobs/booklab/src/tools/booklab')
from theory import parse_pdn, ballot_of
import e2_report as E
d = os.path.expanduser('~/regicide-match/results/' + sys.argv[1])
for p in sorted(glob.glob(d + '/games/*.pdn')):
    for t, m in parse_pdn(open(p).read()):
        col = 'B' if t['Black'].lower().startswith('regicide') else 'W'
        r = {'1-0': 'W' if col == 'B' else 'L', '0-1': 'W' if col == 'W' else 'L'}.get(t['Result'], 'D')
        if r != 'D':
            print(f"{os.path.basename(p)} round {t['Round']} {ballot_of(t['Opening'])[0]} Regicide {col} {r} plies {len(m)} ages A {t.get('EngineGamesA')} B {t.get('EngineGamesB')} term {t.get('Termination')}")
            if r == 'L':
                own = [(i, mv, c) for i, (mv, c) in enumerate(m) if (i % 2 == 0) == (col == 'B')]
                for i, mv, c in own:
                    s = re.search(r'score (-?\d+)', c or '')
                    if s and int(s.group(1)) < -50:
                        print(f"  first own score < -50 at ply {i+1}: {mv} {c}"); break
                print('  moves:', ' '.join(mv for mv, _ in m))
