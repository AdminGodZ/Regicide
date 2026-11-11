# select.py: 64 positions with 10-14 pieces from decisive games (32 Regicide losses, 32 opponent losses).
# f1 games (Book Lab's f1_dbscan table): the last position before the table's first database-decided ply ('first')
# with 10-14 pieces and the eventual loser to move. Other runs (no table here): the last such position in the game.
# At least 2 legal moves for the side to move; unique FENs; games taken round-robin over runs (f1 first).
import csv, glob, gzip, os, collections, random
first = {}
for r in csv.DictReader(gzip.open('f1_dbscan_games.tsv.gz', 'rt'), delimiter='\t'):
    if r['res'] != 'D' and int(r['first']) >= 0: first[(r['run'], r['round'])] = int(r['first'])
cands = {'regicide lost': collections.defaultdict(list), 'opponent lost': collections.defaultdict(list)}
for f in sorted(glob.glob('pdn/*.tsv')):
    run = os.path.basename(f)[:-4]
    games = collections.defaultdict(list)
    for l in open(f):
        g = l.rstrip('\n').split('\t'); games[g[0]].append(g)
    for gid, plies in games.items():
        res, black, white, rnd = plies[-1][5], plies[-1][6], plies[-1][7], plies[-1][8]
        if res not in ('1-0', '0-1'): continue
        loser_colour = 'W' if res == '1-0' else 'B'; loser = white if res == '1-0' else black
        lim = first.get((run, rnd), 10 ** 9) if run.startswith('bl-f1') else 10 ** 9
        pick = None
        for g in plies:
            if int(g[1]) < lim and 10 <= int(g[3]) <= 14 and g[2] == loser_colour and int(g[9]) >= 2: pick = g
        if pick:
            key = 'regicide lost' if 'Regicide' in loser else 'opponent lost'
            src = 'table' if (run, rnd) in first else 'last'
            cands[key][run].append((pick[4], run, rnd, pick[1], pick[3], loser, src, pick[9]))
out = []; seen = set()
for key in cands:
    runs = sorted(cands[key], key=lambda r: (not r.startswith('bl-f1'), r))  # table runs first
    for r in runs: cands[key][r].sort(key=lambda c: (c[6] != 'table', c[2]))
    n = 0; i = 0
    while n < 32 and any(i < len(cands[key][r]) for r in runs):
        for r in runs:
            if i < len(cands[key][r]) and n < 32 and cands[key][r][i][0] not in seen:
                c = cands[key][r][i]; seen.add(c[0]); out.append((key,) + c); n += 1
        i += 1
with open('q64.tsv', 'w') as f:
    f.write('# fen\tgroup\trun\tround\tply\tpieces\tloser\trule\tlegal\n')
    for o in out: f.write('\t'.join(map(str, o[1:2] + o[:1] + o[2:])) + '\n')
print(len(out), collections.Counter(o[0] for o in out), collections.Counter(o[7] for o in out))
print(collections.Counter(o[2] for o in out).most_common(20))
