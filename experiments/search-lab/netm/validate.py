# validate.py MATCH_DIR [--planned-pairs 805]: is a 6-chunk match complete? Per chunk: summary "Games: N of N" without
# "(interrupted)", N = 2 x openings in its chunk file, results.csv has N games, the worker's exit status (c.rc) is 0.
# Prints one line per chunk ("a 278/278 ok") and exits 0 only if every chunk is complete; with --pooled it also needs
# pooled.txt (exit status in pool.rc = 0) to report exactly the planned number of pairs.
import os, re, sys
d = sys.argv[1]; planned = 805; check_pool = '--pooled' in sys.argv
if '--planned-pairs' in sys.argv: planned = int(sys.argv[sys.argv.index('--planned-pairs') + 1])
chunks = os.path.expanduser('~/regicide-jobs/netm/chunks')
ok_all = True; bad = []
for c in 'abcdef':
    want = 2 * sum(1 for l in open(f'{chunks}/op4_{c}') if l.strip())
    cd = os.path.join(d, c); why = []
    try: summ = open(os.path.join(cd, 'summary.txt')).read()
    except OSError: summ = ''; why.append('no summary')
    m = re.search(r'^Games: (\d+) of (\d+)(.*)$', summ, re.M)
    if summ and not m: why.append('no Games line')
    if m:
        if int(m.group(1)) != int(m.group(2)) or 'interrupted' in m.group(3): why.append(f'summary "{m.group(0).strip()}"')
        if int(m.group(2)) != want: why.append(f'scheduled {m.group(2)} != planned {want}')
    try: games = sum(1 for l in open(os.path.join(cd, 'results.csv'))) - 1
    except OSError: games = 0
    if games != want: why.append(f'results.csv {games} games')
    try: rc = open(os.path.join(d, f'{c}.rc')).read().strip()
    except OSError: rc = 'missing'
    # 'unrecorded': the chunk was run by the first runner, which did not record exit statuses; it is written only by
    # restart.sh, and only for chunks whose summary shows every scheduled game played without interruption.
    if rc not in ('0', 'unrecorded'): why.append(f'exit status {rc}')
    note = ' (exit status unrecorded, old runner)' if rc == 'unrecorded' and not why else ''
    print(f'{c} {games}/{want} ' + ('ok' + note if not why else 'INCOMPLETE: ' + '; '.join(why)))
    if why: ok_all = False; bad.append(c)
if check_pool:
    try:
        p = open(os.path.join(d, 'pooled.txt')).read(); prc = open(os.path.join(d, 'pool.rc')).read().strip()
        pairs = int(re.search(r'pairs (\d+)', p).group(1))
    except (OSError, AttributeError): pairs, prc = -1, 'missing'
    good = prc == '0' and pairs == planned
    print(f'pooled pairs {pairs}/{planned} pool exit {prc} ' + ('ok' if good else 'INCOMPLETE'))
    ok_all = ok_all and good
print('COMPLETE' if ok_all else 'INCOMPLETE ' + ''.join(bad))
sys.exit(0 if ok_all else 1)
