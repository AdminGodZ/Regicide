# validate.py MATCH_DIR [--pooled] [--write-pool FILE] [--names A B]
# Strict completeness check of a 6-chunk match, and pooling of validated pairs only.
# Per chunk (planned K = openings in chunks/op4_<c>, 2K games):
#   - summary.txt "Games: 2K of 2K scheduled", not interrupted; worker exit status (<c>.rc) 0 ('unrecorded' only for
#     chunks of the first runner, see restart.sh);
#   - results.csv: game ids unique; for every planned pair p = 1..K exactly two rows with distinct game ids, both with
#     opening "<...>/op4_<c>:p", one with black = A / white = B and one with black = B / white = A; score_a consistent
#     with result (1-0, 0-1, 1/2-1/2) and A's colour; no other rows (no unplanned pair, no third row, no duplicate).
# Names A and B come from --names or from the queue line whose tag is the directory name.
# --pooled: also requires pooled.txt (written by the runner, pool.rc 0) to agree with the strict pool (W/D/L, pairs,
# pentanomial) and 805 pairs. --write-pool FILE: writes the strict pool (pool_dirs.py's format and formula).
import csv, math, os, re, sys
args = sys.argv[1:]; d = args[0].rstrip('/')
NET = os.path.expanduser('~/regicide-jobs/netm'); chunks = os.path.join(NET, 'chunks')
if '--names' in args:
    i = args.index('--names'); name_a, name_b = args[i + 1], args[i + 2]
else:
    tag = os.path.basename(d); name_a = name_b = None
    for l in open(os.path.join(NET, 'queue.txt')):
        f = l.strip().split('|')
        if f[0] == tag: name_a, name_b = f[1], f[2]
    if name_a is None: sys.exit(f'no queue line for {tag}; pass --names A B')
ok_all = True; bad = []; W = D = L = 0; penta = [0] * 5; pairs = []
for c in 'abcdef':
    K = sum(1 for l in open(f'{chunks}/op4_{c}') if l.strip()); want = 2 * K
    cd = os.path.join(d, c); why = []
    try: summ = open(os.path.join(cd, 'summary.txt')).read()
    except OSError: summ = ''; why.append('no summary')
    m = re.search(r'^Games: (\d+) of (\d+)(.*)$', summ, re.M)
    if summ and not m: why.append('no Games line')
    if m and (int(m.group(1)) != want or int(m.group(2)) != want or 'interrupted' in m.group(3)):
        why.append(f'summary "{m.group(0).strip()}" (planned {want})')
    try: rc = open(os.path.join(d, f'{c}.rc')).read().strip()
    except OSError: rc = 'missing'
    if rc not in ('0', 'unrecorded'): why.append(f'exit status {rc}')
    try: rows = list(csv.DictReader(open(os.path.join(cd, 'results.csv'))))
    except OSError: rows = []
    ids = [r['game'] for r in rows]
    if len(set(ids)) != len(ids): why.append(f'{len(ids) - len(set(ids))} duplicate game ids')
    byp = {}
    for r in rows: byp.setdefault(r['pair'], []).append(r)
    extra = set(byp) - {str(p) for p in range(1, K + 1)}
    if extra: why.append(f'unplanned pairs {sorted(extra)[:5]}')
    good_pairs = []
    nbad = 0
    for p in range(1, K + 1):
        rs = byp.get(str(p), [])
        e = []
        if len(rs) != 2: e.append(f'{len(rs)} rows')
        else:
            if rs[0]['game'] == rs[1]['game']: e.append('same game id')
            if any(not r['opening'].endswith(f'/op4_{c}:{p}') for r in rs): e.append('opening')
            roles = sorted((r['black'], r['white']) for r in rs)
            if roles != sorted([(name_a, name_b), (name_b, name_a)]): e.append(f'colours {roles}')
            for r in rs:
                a_black = r['black'] == name_a
                exp = {'1-0': 1.0 if a_black else 0.0, '0-1': 0.0 if a_black else 1.0, '1/2-1/2': 0.5}.get(r['result'])
                try: s = float(r['score_a'])
                except ValueError: s = None
                if exp is None or s != exp: e.append(f'game {r["game"]} score {r["score_a"]} vs result {r["result"]}')
        if e: nbad += 1
        else: good_pairs.append(rs)
    if nbad: why.append(f'{nbad} of {K} pairs missing or invalid')
    print(f'{c} {len(rows)}/{want} games, {len(good_pairs)}/{K} valid pairs ' +
          ('ok' + (' (exit status unrecorded, old runner)' if rc == 'unrecorded' else '') if not why
           else 'INCOMPLETE: ' + '; '.join(why)))
    if why: ok_all = False; bad.append(c)
    for rs in good_pairs:
        v = [float(r['score_a']) for r in rs]
        for s in v: W += s == 1; D += s == 0.5; L += s == 0
        penta[int(round(sum(v) * 2))] += 1; pairs.append(sum(v) / 2)
n = len(pairs)
if n > 1:
    m = sum(pairs) / n; se = math.sqrt(sum((x - m) ** 2 for x in pairs) / (n - 1) / n)
    elo = lambda s: -400 * math.log10(1 / s - 1) if 0 < s < 1 else float('nan')
    slope = 400 / math.log(10) / (m * (1 - m)) if 0 < m < 1 else float('nan')
    z = (m - 0.5) / se if se > 0 else 0; los = 0.5 * (1 + math.erf(z / math.sqrt(2)))
    pool = ("dirs 6  W/D/L %d/%d/%d  pairs %d  pentanomial [LL LD DD/WL WD WW] = %s\n" % (W, D, L, n, penta) +
            "Elo %+.1f [%+.1f, %+.1f] 95%% CI (pentanomial, pairs), score %.4f +- %.4f, LOS %.1f%%\n" % (
                elo(m), elo(m) - 1.96 * se * slope, elo(m) + 1.96 * se * slope, m, 1.96 * se, 100 * los))
else:
    pool = f'dirs 6  W/D/L {W}/{D}/{L}  pairs {n}\n'
if '--write-pool' in args:
    open(args[args.index('--write-pool') + 1], 'w').write(pool)
if '--pooled' in args:
    try:
        p = open(os.path.join(d, 'pooled.txt')).read(); prc = open(os.path.join(d, 'pool.rc')).read().strip()
        same = p.split('\n')[0].strip() == pool.split('\n')[0].strip()
    except OSError: p, prc, same = '', 'missing', False
    good = prc == '0' and same and n == 805
    print(f'strict pool: {n}/805 pairs; runner pool {"agrees" if same else "DIFFERS"}; pool exit {prc} ' +
          ('ok' if good else 'INCOMPLETE'))
    ok_all = ok_all and good
elif n != 805:
    ok_all = False
print('strict pool:', pool.strip().replace('\n', ' | '))
print('COMPLETE' if ok_all else 'INCOMPLETE ' + ''.join(bad))
sys.exit(0 if ok_all else 1)
