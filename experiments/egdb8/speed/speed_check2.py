# EGDB speed check 2, master 769ef0b DLL (partial pre-fault smallest first; set dbmbytes raises the budget of the loaded
# database). Engine dirs with their own egdb/ folder, as used in matches:
#   7-piece : engines/regicide/769ef0b-egdb7 (2-7), default 4096 MB (all 1 GB pre-faulted)
#   8 / 4096: engines/regicide/769ef0b-egdb8 (2-8), default 4096 MB (2-7 + the smallest 8-piece slices pre-faulted)
#   8 / 12288: same + "set dbmbytes 12288" (everything pre-faulted)
# Same 88 positions as speed_check.py; 1 and 8 concurrent cbhost processes; go maxtime 0.5 reset.
import subprocess, time, re, statistics, json, threading
CB = '/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe'
E = 'C:\\regicide-match\\engines\\regicide\\'
CONFIGS = [('7-piece', E + '769ef0b-egdb7\\regicide_cb.dll', None),
           ('8-piece 4096 MB', E + '769ef0b-egdb8\\regicide_cb.dll', None),
           ('8-piece 12288 MB', E + '769ef0b-egdb8\\regicide_cb.dll', 12288)]
pos = [l.split() for l in open('/mnt/c/regicide-match/jobs/egdb8/speed_fens.txt')]

def mem():
    r = subprocess.run(['powershell.exe', '-NoProfile', '-Command',
                        'Get-Process cbhost -ErrorAction SilentlyContinue | ForEach-Object { "$($_.WorkingSet64) $($_.PrivateMemorySize64)" }'],
                       capture_output=True, text=True).stdout.split()
    v = list(map(int, r)); ws, pm = v[0::2], v[1::2]
    return f'cbhost x{len(ws)}: working set max {max(ws)/2**30:.2f} GB/proc, private {sum(pm)/len(pm)/2**30:.2f} GB/proc' if ws else 'no cbhost'

def start(dll, mb):
    p = subprocess.Popen([CB, dll], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)
    def cmd(s):
        p.stdin.write(s + '\n'); p.stdin.flush()
        while True:
            l = p.stdout.readline().rstrip('\r\n')
            if l.startswith(('reply', 'ok', 'error', 'bestmove')) or l == '': return l
    while p.stdout.readline().strip() != 'ready': pass
    t0 = time.time(); first = cmd('cmd get dbpieces'); t_load = time.time() - t0   # first command: default load
    cmd('cmd set threads 1'); cmd('cmd set hashsize 128')
    t_mb = 0.0
    if mb:
        t0 = time.time(); cmd(f'cmd set dbmbytes {mb}'); t_mb = time.time() - t0
    return p, cmd, first, t_load, t_mb

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
        res.append((int(n), int(m.group(1)) if m else 0, t))
    return res

out = {}
for rnd in (1, 2):
    for name, dll, mb in CONFIGS:
        for k in (8, 1):
            procs = [start(dll, mb) for _ in range(k)]
            m1 = mem()
            results = [None] * k
            th = [threading.Thread(target=lambda i=i: results.__setitem__(i, run(procs[i][0], procs[i][1]))) for i in range(k)]
            [t.start() for t in th]; time.sleep(20); m2 = mem(); [t.join() for t in th]
            for p in procs: p[0].stdin.write('quit\n'); p[0].stdin.flush(); p[0].wait()
            rows = [r for rr in results for r in rr]
            def nps(sel):
                s = [r for r in rows if sel(r[0]) and r[2] >= 0.2 and r[1] > 0]
                return (sum(r[1] for r in s) / sum(r[2] for r in s), len(s)) if s else (0, 0)
            line = (f'round {rnd} {name:17s} x{k}: {procs[0][2]}; first-command load {statistics.mean(x[3] for x in procs):.2f} s'
                    f'{"; set dbmbytes %.2f s" % statistics.mean(x[4] for x in procs) if mb else ""}; nps: ')
            for lab, sel in [('9-10', lambda n: 9 <= n <= 10), ('11-12', lambda n: 11 <= n <= 12), ('16-20', lambda n: n >= 16),
                             ('all', lambda n: True)]:
                v, c = nps(sel); line += f'{lab} {v/1e6:.2f}M ({c})  '
            print(line, flush=True)
            print(f'   memory after load: {m1}; during search: {m2}', flush=True)
            out[f'{rnd} {name} x{k}'] = rows
json.dump(out, open('/mnt/c/regicide-match/jobs/egdb8/speed2_raw.json', 'w'))
