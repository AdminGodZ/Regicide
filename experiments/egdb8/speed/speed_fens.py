# Positions for the EGDB speed check: walk the 8-piece baseline games (KingsRow, Cake, GuiNN) with the Linux CLI,
# keep non-capture positions by piece count. Output: speed_fens.txt lines "<pieces> <FEN>".
import re, glob, subprocess, os, random
E = os.path.expanduser('~/regicide-cc9/build/regicide')
tok = re.compile(r'^\d+(?:[-x]\d+)+$')
random.seed(7)
games = []
for opp in ('kingsrow', 'cake', 'guinn'):
    games += sorted(glob.glob(os.path.expanduser(f'~/regicide-match/results/baseline-30f09c3-egdb8-{opp}/games/*.pdn')))
random.shuffle(games)
cmds, meta = [], []
for g in games[:120]:
    body = re.sub(r'\{[^}]*\}', ' ', re.sub(r'\[[^\]]*\]', ' ', open(g).read()))
    mv = [t for t in body.split() if tok.match(t)]
    for ply in range(20, len(mv) - 1, 3):
        cmds.append('position startpos moves ' + ' '.join(mv[:ply]) + '\nd\n')
out = subprocess.run([E], input=''.join(cmds) + 'quit\n', capture_output=True, text=True).stdout
fens = re.findall(r'^([BW]:[^\s]+)$', out, re.M)
seen, buckets = set(), {}
for f in fens:
    if f in seen: continue
    seen.add(f)
    n = len(re.findall(r'K?\d+', f))
    buckets.setdefault(n, []).append(f)
pick = []
for n, k in [(7, 8), (8, 8), (9, 16), (10, 16), (11, 12), (12, 12), (16, 8), (20, 8)]:
    pick += [(n, f) for f in random.sample(buckets.get(n, []), min(k, len(buckets.get(n, []))))]
with open('/mnt/c/regicide-match/jobs/egdb8/speed_fens.txt', 'w') as fo:
    for n, f in pick: fo.write(f'{n} {f}\n')
print({n: len(v) for n, v in sorted(buckets.items())}, 'picked', len(pick))
