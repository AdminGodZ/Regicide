# Search nps on real game positions with the 2-7 vs the 2-8 piece database, Windows regicide.exe (30f09c3 build).
# Each case runs N copies at once (N=1 and N=8) to see whether concurrency on the mapped files matters.
import re, glob, subprocess, statistics, os, sys
E = '/mnt/c/regicide-match/wintest/30f09c3/regicide.exe'
DBS = {'7': 'C:\\regicide-match\\engines\\regicide\\30f09c3\\egdb', '8': 'C:\\regicide-match\\engines\\regicide\\30f09c3-egdb8\\egdb'}
games = sorted(glob.glob(os.path.expanduser('~/regicide-match/results/baseline-30f09c3-egdb8-kingsrow/games/*.pdn')))[:40]
tok = re.compile(r'^\d+(?:[-x]\d+)+$')
cmds = []
for g in games:
    body = re.sub(r'\{[^}]*\}', ' ', re.sub(r'\[[^\]]*\]', ' ', open(g).read()))
    mv = [t for t in body.split() if tok.match(t)]
    for ply in (24, 36, 48):
        if len(mv) > ply + 2:
            cmds.append('position startpos moves ' + ' '.join(mv[:ply]) + '\ngo movetime 500\n')
script = ''.join(c + 'newgame\n' for c in cmds) + 'quit\n'
def parse(out):
    last, prev = [], None
    for l in out.replace('\r', '').split('\n'):
        if l.startswith('bestmove'): last.append(prev); continue
        if 'nps' in l: prev = l
    return [int(re.search(r'nps (\d+)', l).group(1)) for l in last if l and re.search(r'nps (\d+)', l)]
print('positions', len(cmds))
for n in (1, 8):
    for k, d in DBS.items():
        env = dict(os.environ, REGICIDE_EGDB=d, WSLENV='REGICIDE_EGDB')
        ps = [subprocess.Popen([E], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, env=env) for _ in range(n)]
        outs = [p.communicate(script)[0] for p in ps]
        nps = [x for o in outs for x in parse(o)]
        egl = [l for l in outs[0].replace('\r', '').split('\n') if l.startswith('egdb')]
        print(f'{n} process(es), egdb {k}: {egl[:1]} searches {len(nps)}, median nps {statistics.median(nps):,.0f}, mean {statistics.mean(nps):,.0f}', flush=True)
