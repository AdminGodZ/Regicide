# report.py: dips test table. Scores per position at 1M/3M/10M for each config; "within 30" of Book Lab's 100M score
# (and of this machine's own 100M baseline when present).
import csv, os, re, sys
D = os.path.expanduser('~/regicide-jobs/dips')
rows = list(csv.DictReader(open(f'{D}/dips.tsv'), delimiter='\t'))
def load(name, n):
    f = f'{D}/out/{name}_n{n}.txt'
    if not os.path.exists(f): return None
    out = {}
    for l in open(f):
        m = re.match(r'(\d+) (\S+) depth (\d+) score (-?\d+) nodes (\d+) ms (\d+) move (\S+)', l)
        if m: out[int(m.group(1))] = (int(m.group(4)), m.group(7), int(m.group(3)))
    return out if len(out) == len(rows) else None
own = load('base', 100000000)
cfgs = [('base', 'baseline'), ('lmr80', 'lmr_base=80'), ('lmr120', 'lmr_base=120'), ('pf3', 'prune_forcing=3')]
ns = [1000000, 3000000, 10000000]
print('| # | game | pieces | game score (0.5 s) | Book Lab 100M | own 100M (baseline) | ' +
      ' | '.join(f'{lab} {n // 10**6}M' for _, lab in cfgs for n in ns) + ' |')
print('|' + '---|' * (6 + len(cfgs) * len(ns)))
res = {(c, n): load(c, n) for c, _ in cfgs for n in ns}
for i, r in enumerate(rows):
    pieces = sum(len([x for x in p[1:].split(',') if x]) for p in r['fen'].split(':')[1:])
    cells = []
    for c, _ in cfgs:
        for n in ns:
            v = res[(c, n)][i][0] if res[(c, n)] else None
            cells.append('—' if v is None else f'{v:+d}' + ('' if abs(v - int(r['score_100m'])) <= 30 else ' ✗'))
    o = f'{own[i][0]:+d} ({own[i][1]})' if own else 'running'
    print(f'| {i} | {r["games (run:round)"]} ply {r["ply"]} | {pieces} | {int(r["game_score"]):+d} | {int(r["score_100m"]):+d} ({r["best_100m"]}) | {o} | ' + ' | '.join(cells) + ' |')
print()
print('| config | within 30 of Book Lab 100M: 1M / 3M / 10M | within 30 of own 100M: 1M / 3M / 10M | mean |score − Book Lab 100M| 1M / 3M / 10M |')
print('|---|---|---|---|')
for c, lab in cfgs:
    w = []; wo = []; md = []
    for n in ns:
        r_ = res[(c, n)]
        if not r_: w.append('—'); wo.append('—'); md.append('—'); continue
        w.append(str(sum(1 for i, r in enumerate(rows) if abs(r_[i][0] - int(r['score_100m'])) <= 30)))
        wo.append(str(sum(1 for i in range(len(rows)) if abs(r_[i][0] - own[i][0]) <= 30)) if own else '—')
        md.append(f'{sum(abs(r_[i][0] - int(r["score_100m"])) for i, r in enumerate(rows)) / len(rows):.0f}')
    print(f'| {lab} | {" / ".join(w)} of {len(rows)} | {" / ".join(wo)} | {" / ".join(md)} |')
