# EGDB speed check through cbhost.exe + the CheckerBoard DLL (master 65f7143), on positions from the 8-piece baseline.
# The DLL dir has no egdb/ folder, so the default load at the first command fails and the database comes only from
#   cmd set dbmbytes <MB>   (cache size for the next load)
#   cmd set dbpath <dir>    (egdb::init(dir, cache_mb): pre-faults everything iff the total fits in cache_mb)
# Configs: 7-piece (2-7 copy, 4096 MB: 1 GB pre-faulted), 8-piece at 4096 MB (on demand), 8-piece at 16384 MB
# (pre-faulted). Each with 1 and with 8 concurrent cbhost processes; 0.5 s searches (go maxtime 0.5 reset).
import subprocess, time, re, sys, statistics, json
CB = '/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe'
DLL = 'C:\\regicide-match\\engines\\regicide\\65f7143-noegdb\\regicide_cb.dll'
DB7 = 'C:\\regicide-match\\engines\\regicide\\30f09c3\\egdb'
DB8 = 'C:\\regicide-match\\engines\\regicide\\65f7143-egdb8\\egdb'
CONFIGS = [('7-piece', DB7, 4096), ('8-piece 4096 MB', DB8, 4096), ('8-piece 16384 MB', DB8, 16384)]
pos = [l.split() for l in open('/mnt/c/regicide-match/jobs/egdb8/speed_fens.txt')]

def mem():
    r = subprocess.run(['powershell.exe', '-NoProfile', '-Command',
                        'Get-Process cbhost -ErrorAction SilentlyContinue | ForEach-Object { "$($_.WorkingSet64) $($_.PrivateMemorySize64)" }'],
                       capture_output=True, text=True).stdout.split()
    v = list(map(int, r))
    ws, pm = v[0::2], v[1::2]
    return f'cbhost x{len(ws)}: working set {sum(ws)/2**30:.2f} GB total (max {max(ws)/2**30:.2f}), private {sum(pm)/2**30:.2f} GB total' if ws else 'no cbhost'

def start(db, mb):
    p = subprocess.Popen([CB, DLL], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)
    def cmd(s):
        p.stdin.write(s + '\n'); p.stdin.flush()
        while True:
            l = p.stdout.readline().rstrip('\r\n')
            if l.startswith(('reply', 'ok', 'error', 'bestmove')) or l == '': return l
    while p.stdout.readline().strip() != 'ready': pass
    cmd('cmd set threads 1'); cmd('cmd set hashsize 128')
    first = cmd('cmd get dbpieces')              # the first command triggers the (failing) default load
    cmd(f'cmd set dbmbytes {mb}')
    t0 = time.time(); r = cmd(f'cmd set dbpath {db}'); t_load = time.time() - t0
    return p, cmd, first, r, t_load

def run(p, cmd):
    res = []
    for n, fen in pos:
        cmd('newgame'); cmd(f'position fen {fen}')
        p.stdin.write('go maxtime 0.5 reset\n'); p.stdin.flush()
        info = ''
        while True:
            l = p.stdout.readline().rstrip('\r\n')
            if l.startswith('info'): info = l
            if l.startswith(('bestmove', 'error')): break
        t = float(re.search(r'time ([\d.]+)', l).group(1)) if 'time' in l else 0
        m = re.search(r'nodes (\d+)', info)
        res.append((int(n), int(m.group(1)) if m else 0, t, info[:90]))
    return res

out = {}
for name, db, mb in CONFIGS:
    for k in (1, 8):
        procs = [start(db, mb) for _ in range(k)]
        loads = [x[4] for x in procs]
        m1 = mem()
        import threading
        results = [None] * k
        th = [threading.Thread(target=lambda i=i: results.__setitem__(i, run(procs[i][0], procs[i][1]))) for i in range(k)]
        t0 = time.time(); [t.start() for t in th]; time.sleep(20); m2 = mem(); [t.join() for t in th]; wall = time.time() - t0
        for p in procs: p[0].stdin.write('quit\n'); p[0].stdin.flush(); p[0].wait()
        rows = [r for rr in results for r in rr]
        def nps(sel):
            s = [r for r in rows if sel(r[0]) and r[2] >= 0.2 and r[1] > 0]
            return (sum(r[1] for r in s) / sum(r[2] for r in s), len(s)) if s else (0, 0)
        line = (f'{name:18s} x{k}: dbpieces-after-default "{procs[0][2]}", set dbpath -> "{procs[0][3]}", '
                f'load {statistics.mean(loads):.2f} s (max {max(loads):.2f}); searched-move nps: ')
        for lab, sel in [('7-8 pc', lambda n: n <= 8), ('9-10', lambda n: 9 <= n <= 10), ('11-12', lambda n: 11 <= n <= 12),
                         ('16-20', lambda n: n >= 16), ('all', lambda n: True)]:
            v, c = nps(sel); line += f'{lab} {v/1e6:.2f}M ({c})  '
        print(line, flush=True)
        print(f'   memory after load: {m1}; during search: {m2}; wall {wall:.0f} s', flush=True)
        out[f'{name} x{k}'] = rows
json.dump(out, open('/mnt/c/regicide-match/jobs/egdb8/speed_raw.json', 'w'))
