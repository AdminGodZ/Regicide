# Search nps on real game positions with the 2-7 vs the 2-8 piece database (Linux build of master cc9f35f, n4c).
import re, glob, subprocess, statistics, os
E = os.path.expanduser('~/regicide-cc9/build/regicide')
DBS = {'7': os.path.expanduser('~/regicide-jobs/egdb8/egdb7view'), '8': os.path.expanduser('~/regicide-data/egdb')}
games = sorted(glob.glob(os.path.expanduser('~/regicide-match/results/baseline-30f09c3-egdb8-kingsrow/games/*.pdn')))[:40]
tok = re.compile(r'^\d+(?:[-x]\d+)+$')
cmds = []
for g in games:
    body = open(g).read().split('\n\n', 1)[-1] if '\n\n' in open(g).read() else open(g).read()
    body = re.sub(r'\{[^}]*\}', ' ', re.sub(r'\[[^\]]*\]', ' ', open(g).read()))
    mv = [t for t in body.split() if tok.match(t)]
    for ply in (24, 36, 48):
        if len(mv) > ply + 2:
            cmds.append('position startpos moves ' + ' '.join(mv[:ply]) + '\ngo movetime 500\n')
print('positions', len(cmds))
res = {}
for rnd in (1, 2):
    for k, d in DBS.items():
        script = ''.join(c + 'newgame\n' for c in cmds) + 'quit\n'
        out = subprocess.run([E], input=script, capture_output=True, text=True, env=dict(os.environ, REGICIDE_EGDB=d)).stdout
        lines = out.split('\n')
        last = []
        prev = None
        for l in lines:
            if l.startswith('bestmove'):
                last.append(prev); continue
            if 'nps' in l: prev = l
        nps = [int(re.search(r'nps (\d+)', l).group(1)) for l in last if l and re.search(r'nps (\d+)', l)]
        ill = out.count('illegal')
        res.setdefault(k, []).extend(nps)
        print(f'round {rnd} egdb {k}: {len(nps)} searches, illegal {ill}, median nps {statistics.median(nps):,.0f}, mean {statistics.mean(nps):,.0f}')
        if rnd == 1 and k == '7': print('sample info line:', last[0][:200] if last and last[0] else None)
