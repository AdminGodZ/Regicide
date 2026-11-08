# Summarise the Cake lossprobe report: per lost game the blamed move (ply, move number after the 3-move ballot, piece
# count, phase, drop, margin); counts by phase; ballots and blamed positions that recur across the three runs.
# Phases: opening = within the first 10 moves after the ballot (ply - ballot < 20); else middlegame if > 12 pieces,
# endgame if <= 12 pieces.
import re, collections, sys
rep = open(sys.argv[1] if len(sys.argv) > 1 else '/home/admingod/regicide-jobs/cake/report_cake_769ef0b.txt').read()
games = re.split(r'^== ', rep, flags=re.M)[1:]
rows = []
for g in games:
    head = g.split('\n', 1)[0]
    m = re.search(r'#(\d+): .*?  (\S+)  (ACF #\d+[^:]*):', head)
    num, res, ballot = int(m.group(1)), m.group(2), m.group(3).strip()
    colour = re.search(r'Regicide[^(]*\((black|white)\)', head).group(1)
    b = re.search(r'BLAME ply (\d+) (\d+\.(?:\.\.)?)\s+(\S+)\s+FEN (\S+)', g)
    if not b:
        rows.append(dict(n=num, colour=colour, ballot=ballot, blame=None, why='unclear' if 'unclear' in g.lower() else '?'))
        continue
    ply, mv, fen = int(b.group(1)), b.group(3), b.group(4)
    pcs = len(re.findall(r'K?\d+', fen))
    d = re.search(r'drop (\d+) from the previous score (-?\S+), margin (\d+)', g)
    after = ply - 6
    phase = 'opening' if after < 20 else ('endgame' if pcs <= 12 else 'middlegame')
    played = re.search(r'played \S+ (\S+)', g).group(1)
    alt = re.search(r'best alternative \S+ (\S+)', g).group(1)
    rows.append(dict(n=num, colour=colour, ballot=ballot, blame=ply, after=after // 2 + 1, move=mv, fen=fen, pcs=pcs,
                     phase=phase, drop=int(d.group(1)) if d else None, prev=d.group(2) if d else None,
                     margin=int(d.group(3)) if d else None, played=played, alt=alt))
runs = ['off'] * 11 + ['avoid'] * 11 + ['bookavoid'] * 13      # order of cake_losses_769ef0b.pdn
for r, run in zip(rows, runs): r['run'] = run
bl = [r for r in rows if r['blame']]
print(f'{len(rows)} games, {len(bl)} blamed, {len(rows) - len(bl)} unclear')
print('phase of the blamed move:', dict(collections.Counter(r['phase'] for r in bl)))
af = sorted(r['after'] for r in bl)
print(f'blamed move number after the ballot: median {af[len(af)//2]}, quartiles {af[len(af)//4]}-{af[3*len(af)//4]}, range {af[0]}-{af[-1]}')
print('buckets (moves after ballot):', dict(collections.Counter(('1-5' if a <= 5 else '6-10' if a <= 10 else '11-20' if a <= 20 else '21-30' if a <= 30 else '31+') for a in af)))
pc = collections.Counter(r['pcs'] for r in bl)
print('pieces at the blamed move:', dict(sorted(pc.items())))
print('blames into database losses (played score dbloss):', sum('dbloss' in r['played'] for r in bl))
print('colour:', dict(collections.Counter(r['colour'] for r in rows)))
bc = collections.Counter(r['ballot'] for r in rows)
print('ballots lost in more than one run:', {k: v for k, v in bc.most_common() if v > 1})
fc = collections.Counter((r['fen'], r['move']) for r in bl)
print('blamed (position, move) pairs:', len(fc), '; repeated:', sum(1 for v in fc.values() if v > 1))
for (f, m), v in fc.most_common():
    if v > 1:
        rr = [r for r in bl if r['fen'] == f and r['move'] == m]
        print(f'   x{v}  {m:7s} {rr[0]["ballot"]:26s} after-ballot move {rr[0]["after"]:2d}, {rr[0]["pcs"]} pcs, runs {",".join(r["run"] for r in rr)}  {f}')
print()
print(f'{"run":10s} {"#":>3s} {"col":5s} {"ballot":26s} {"ply":>4s} {"mv":>3s} {"pcs":>4s} {"phase":10s} {"move":8s} {"played":>10s} {"alt":>10s} {"drop":>5s}')
for r in sorted(rows, key=lambda r: (r['ballot'], r['run'])):
    if not r['blame']:
        print(f'{r["run"]:10s} {r["n"]:3d} {r["colour"]:5s} {r["ballot"][:26]:26s}  unclear'); continue
    print(f'{r["run"]:10s} {r["n"]:3d} {r["colour"]:5s} {r["ballot"][:26]:26s} {r["blame"]:4d} {r["after"]:3d} {r["pcs"]:4d} {r["phase"]:10s} '
          f'{r["move"]:8s} {r["played"]:>10s} {r["alt"]:>10s} {r["drop"]:5d}')
