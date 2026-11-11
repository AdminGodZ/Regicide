import sys, glob, os, re, collections
sys.path.insert(0, '/mnt/c/regicide-match/jobs/booklab/src/tools/booklab')
from theory import parse_pdn
import e2_report as E
d = os.path.expanduser('~/regicide-match/results/bl-e2d-cake163-natural')
gs = []
for p in sorted(glob.glob(d + '/games/*.pdn')):
    for t, m in parse_pdn(open(p).read()):
        if t['White'].lower().startswith('regicide'):
            r = {'0-1': 'W', '1-0': 'L'}.get(t['Result'], 'D')
            gs.append(dict(f=os.path.basename(p), w=t.get('Worker'), age=t.get('EngineGamesB'), r=r, mv=[x for x, _ in m], c=[y for _, y in m]))
won = [g for g in gs if g['r'] == 'W']; drawn = [g for g in gs if g['r'] == 'D']
print('White games', len(gs), 'won', len(won), 'drawn', len(drawn), 'lost', sum(g['r'] == 'L' for g in gs))
print('ages won', sorted(int(g['age']) for g in won), 'drawn', sorted(int(g['age']) for g in drawn))
# first ply where the won and drawn sets separate: for each won game, the first ply at which its prefix matches no drawn game
def sep(g, others):
    for p in range(len(g['mv'])):
        if not any(o['mv'][:p + 1] == g['mv'][:p + 1] for o in others):
            return p
    return None
rows = collections.Counter()
for g in won:
    p = sep(g, drawn)
    who = 'Cake' if p % 2 == 0 else 'Regicide'
    alts = sorted({o['mv'][p] for o in drawn if o['mv'][:p] == g['mv'][:p] and len(o['mv']) > p})
    print(f"won {g['f']} worker {g['w']} age {g['age']}: leaves all drawn games at ply {p+1} ({who}) with {g['mv'][p]} {E.dn(g['c'][p])}; drawn games there: {alts} ({sum(1 for o in drawn if o['mv'][:p] == g['mv'][:p])} games)")
for g in drawn:
    p = sep(g, won)
    who = 'Cake' if p is not None and p % 2 == 0 else 'Regicide'
    alts = sorted({o['mv'][p] for o in won if p is not None and o['mv'][:p] == g['mv'][:p] and len(o['mv']) > p})
    print(f"drawn {g['f']} worker {g['w']} age {g['age']}: leaves all won games at ply {p+1 if p is not None else '-'} ({who}) with {g['mv'][p] if p is not None else '-'} {E.dn(g['c'][p]) if p is not None else ''}; won games there: {alts}")
