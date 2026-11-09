# Targeted book_deep2 vs book_deep runs (ba-d2t*-cake / ba-d1t*-cake): losses per ballot (identified by the first three
# moves of the game) and Regicide's colour.
import csv, glob, os, re, collections
R = os.path.expanduser('~/regicide-match/results')
NAMES = {'9-13 22-18 10-15': 'ACF #7', '11-16 23-19 16x23': 'ACF #153 Black Widow', '9-14 23-18 14x23': 'ACF #46 Double Cross',
         '10-14 22-18 12-16': 'ACF #68 White Doctor', '9-13 23-18 11-16': 'ACF #16 Wilderness II'}
for tag in ('d2t', 'd2bt', 'd1t'):
    c, n = collections.Counter(), collections.Counter()
    runs = sorted(d for d in glob.glob(f'{R}/ba-{tag}[0-9]*-cake') if os.path.exists(f'{d}/summary.txt') and 'Elo (pent' in open(f'{d}/summary.txt').read())
    for d in runs:
        for r in csv.DictReader(open(f'{d}/results.csv')):
            t = open(f'{d}/games/{int(r["game"]):05d}.pdn').read()
            col = 'B' if re.search(r'\[Black "Regicide', t) else 'W'
            body = re.sub(r'\{[^}]*\}', ' ', re.sub(r'\[[^\]]*\]', ' ', t))
            mv = [x for x in body.split() if re.match(r'^\d+[-x]\d+', x)][:3]
            k = (NAMES.get(' '.join(mv), ' '.join(mv)), col); n[k] += 1
            if r['score_a'] == '0': c[k] += 1
    print(f'{dict(d2t="book_deep2", d2bt="book_deep2b", d1t="book_deep")[tag]}: {len(runs)} runs; Regicide losses / games per ballot and colour:')
    for k in sorted(n):
        print(f'   {k[0]:24s} {k[1]}  {c[k]:3d} / {n[k]:3d}')
