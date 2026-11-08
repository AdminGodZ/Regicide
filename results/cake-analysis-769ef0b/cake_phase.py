# Cake losses of the 769ef0b runs (off/avoid/bookavoid at 0.5 s): ballot, colour, and the decisive moment = the first
# Cake move whose own eval (value=) is >= +100 and never drops below +50 afterwards. For that moment: Regicide move number
# after the ballot, piece count (replayed with the Linux CLI), Regicide's own score on its previous move. Phases: opening
# = within 10 moves after the 3-move ballot (ply < 6 + 20); middlegame = later with > 12 pieces; endgame = <= 12 pieces.
# Also writes all the losses into one PDN for lossprobe (cake_losses_769ef0b.pdn).
import re, glob, os, csv, subprocess, collections, sys
R = os.path.expanduser('~/regicide-match/results')
E = os.path.expanduser('~/regicide-cc9/build/regicide')
OUT = '/mnt/c/regicide-match/jobs/cake'
runs = sys.argv[1:] or ['off-769', 'avoid-769', 'bookavoid-769']
tok = re.compile(r'^\d+(?:[-x]\d+)+$')
rows, pdn = [], []
for run in runs:
    d = f'{R}/ba-{run}-cake'
    for r in csv.DictReader(open(f'{d}/results.csv')):
        if r['score_a'] != '0': continue
        txt = open(f'{d}/games/{int(r["game"]):05d}.pdn').read()
        pdn.append(txt.rstrip('\n') + '\n')
        reg_black = re.search(r'\[Black "Regicide', txt) is not None
        body = re.sub(r'\[[^\]]*\]', ' ', txt)
        items = re.findall(r'\{[^}]*\}|\S+', body)
        moves, comm = [], {}
        for it in items:
            if it.startswith('{'):
                if moves: comm[len(moves) - 1] = it.replace('\n', ' ')
            elif tok.match(it): moves.append(it)
        cake_vals, reg_scores = {}, {}
        for i, mv in enumerate(moves):
            c = comm.get(i, '')
            reg_move = (i % 2 == 0) == reg_black
            if reg_move:
                m = re.search(r' score (-?\d+)', c)
                if m: reg_scores[i] = int(m.group(1))
            else:
                m = re.search(r'value=(-?\d+)', c)
                if m: cake_vals[i] = int(m.group(1))
        dec = None
        ks = sorted(cake_vals)
        for j, i in enumerate(ks):
            if cake_vals[i] >= 100 and all(cake_vals[k] >= 50 for k in ks[j:]):
                dec = i; break
        rows.append(dict(run=run, game=int(r['game']), opening=r['opening'], colour='B' if reg_black else 'W',
                         plies=len(moves), dec=dec, moves=moves, reg=reg_scores, cake=cake_vals))
# piece counts at the decisive ply
cmds = [('position startpos moves ' + ' '.join(x['moves'][:x['dec']]) + '\nd\n') for x in rows if x['dec'] is not None]
out = subprocess.run([E], input=''.join(cmds) + 'quit\n', capture_output=True, text=True).stdout
fens = iter(re.findall(r'^([BW]:[^\s]+)$', out, re.M))
for x in rows:
    if x['dec'] is None: x['pieces'] = None; continue
    x['pieces'] = len(re.findall(r'K?\d+', next(fens)))
    after = (x['dec'] - 6) // 2 + 1                      # move number after the ballot (1 = first move after it)
    x['after'] = after
    x['phase'] = 'opening' if x['dec'] < 26 else ('endgame' if x['pieces'] <= 12 else 'middlegame')
    prev = [i for i in x['reg'] if i < x['dec']]
    x['regscore'] = x['reg'][max(prev)] if prev else None
open(f'{OUT}/cake_losses_769ef0b.pdn', 'w').write('\n'.join(pdn))
print(f'{len(rows)} lost games; per run:', dict(collections.Counter(x["run"] for x in rows)))
print('colour (Regicide):', dict(collections.Counter(x['colour'] for x in rows)))
print('phase of the decisive moment:', dict(collections.Counter(x.get('phase', 'none') for x in rows if x['dec'] is not None)),
      ' no clear jump:', sum(x['dec'] is None for x in rows))
op = collections.Counter(x['opening'] for x in rows)
print('ballots with more than one loss:', {k: v for k, v in op.most_common() if v > 1})
print('distinct ballots:', len(op))
print(f'{"run":14s} {"game":>5s} {"col":3s} {"ballot":28s} {"plies":>5s} {"dec ply":>7s} {"mv after":>8s} {"pcs":>4s} {"phase":10s} {"cake":>6s} {"regicide":>8s}')
for x in sorted(rows, key=lambda x: (x['opening'], x['run'])):
    if x['dec'] is None:
        print(f'{x["run"]:14s} {x["game"]:5d} {x["colour"]:3s} {x["opening"][:28]:28s} {x["plies"]:5d}   (no sustained Cake eval >= 100)'); continue
    print(f'{x["run"]:14s} {x["game"]:5d} {x["colour"]:3s} {x["opening"][:28]:28s} {x["plies"]:5d} {x["dec"]:7d} {x["after"]:8d} {x["pieces"]:4d} '
          f'{x["phase"]:10s} {x["cake"][x["dec"]]:6d} {str(x["regscore"]):>8s}')
