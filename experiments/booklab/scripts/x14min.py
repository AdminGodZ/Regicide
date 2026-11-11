import csv, collections
sc = {}
for l in open('/mnt/c/regicide-match/jobs/booklab/wp14/step1.out'):
    f = l.rstrip('\n').split('\t')
    if f[1] == 'root': sc[f[0]] = (f[2], int(f[3]) if f[3] != 'none' else None, int(f[4]))
print('| context | game lowest (0.5 s) | 100M root: lowest (ply, best move, depth) | 100M at the game\'s lowest ply |')
print('|---|---|---|---|')
for c in csv.DictReader(open('/mnt/c/regicide-match/jobs/booklab/wp14/contexts.tsv'), delimiter='\t'):
    rows = [(int(t.split(':')[1]), sc.get(t.split(':')[0])) for t in c['tasks'].split()]
    rows = [(p, s) for p, s in rows if s and s[1] is not None]
    lo = min(rows, key=lambda r: r[1][1])
    at = next((s for p, s in rows if p == int(c['lowest_ply'])), None)
    print(f"| {c['context']} | {c['lowest_score']} @ {c['lowest_ply']} | {lo[1][1]} (ply {lo[0]}, {lo[1][0]}, d{lo[1][2]}) | {at[1] if at else '-'} (d{at[2] if at else '-'}) |")
