#!/usr/bin/env python3
"""cbhost checks of the ponder DLL (run in WSL; cbhost.exe runs on Windows through interop).
cbcheck.py DLLDIR  -> prints one line per check, exit 1 if any failed."""
import subprocess, sys, time, threading, queue

CB = '/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe'
E = 'C:\\regicide-match\\engines\\regicide\\'
dlldir = sys.argv[1]
fails = []


class Host:
    def __init__(self, opts):
        self.p = subprocess.Popen([CB, E + dlldir + '\\regicide_cb.dll'], stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True, bufsize=1)
        self.q = queue.Queue()
        threading.Thread(target=self._reader, daemon=True).start()
        self.expect('ready', 30)
        for o in opts:
            self.send('cmd ' + o)
            self.expect('reply', 10)

    def _reader(self):
        for line in self.p.stdout:
            self.q.put(line.rstrip('\n'))
        self.q.put(None)

    def send(self, s):
        self.p.stdin.write(s + '\n')
        self.p.stdin.flush()

    def expect(self, prefix, timeout):
        end = time.time() + timeout
        seen = []
        while True:
            try:
                line = self.q.get(timeout=max(0.01, end - time.time()))
            except queue.Empty:
                raise TimeoutError(f'no {prefix!r} within {timeout}s; seen {seen[-3:]}')
            if line is None:
                raise EOFError(f'cbhost exited; seen {seen[-3:]}')
            seen.append(line)
            if line.startswith(prefix):
                return line, seen

    def cmd(self, c):
        self.send('cmd ' + c)
        return self.expect('reply', 10)[0]

    def go(self, moves, t=0.5, reset=False, newgame=False):
        if newgame:
            self.send('newgame'); self.expect('ok', 10)
        self.send('position startpos' + (' moves ' + ' '.join(moves) if moves else ''))
        self.expect('ok', 10)
        t0 = time.time()
        self.send(f'go maxtime {t}' + (' reset' if reset else ''))
        line, seen = self.expect('bestmove', 10 * t + 5)
        info = [s for s in seen if s.startswith('info')]
        return line.split()[1], (info[-1] if info else ''), time.time() - t0


def check(name, ok, detail=''):
    print(('PASS ' if ok else 'FAIL ') + name + (': ' + detail if detail else ''), flush=True)
    if not ok:
        fails.append(name)


def pv_of(info):
    toks = info.split()
    return toks[toks.index('pv') + 1:] if 'pv' in toks else []


BALLOT = ['11-15', '23-19', '8-11']

# 1. ponder 0: plays normally, no ponder thread, no ponder note
h = Host(['set hashsize 128', 'set threads 1', 'set book 0'])
check('ponder off by default', h.cmd('get ponder').endswith('1 0'), h.cmd('get ponder'))
mv, info, dt = h.go(BALLOT, reset=True, newgame=True)
pp = h.cmd('get ponderpos')
check('ponder 0: move in time, no ponder note, no ponder search',
      dt < 1.0 and 'ponder' not in info and pp.strip() == 'reply 1', f'{mv} {dt:.2f}s [{info}] [{pp}]')
pv = pv_of(info)
mv2, info2, dt2 = h.go(BALLOT + [mv, pv[1]])
check('ponder 0: second move, no ponder note', dt2 < 1.0 and 'ponder' not in info2, f'{dt2:.2f}s [{info2}]')
t0 = time.time(); h.send('quit'); h.p.wait(10)
check('ponder 0: quit', h.p.returncode is not None, f'{time.time() - t0:.2f}s')

# 2. ponder 1, threads 4: hit, miss, new game while pondering, quit while pondering
h = Host(['set hashsize 512', 'set threads 4', 'set book 0', 'set ponder 1'])
mv, info, dt = h.go(BALLOT, reset=True, newgame=True)
pv = pv_of(info)
time.sleep(0.05)
pp = h.cmd('get ponderpos')
check('ponder 1: ponder search starts after the move', len(pp.split()) == 3, f'{mv} pv {pv[:3]} [{pp}]')
time.sleep(0.5)  # the opponent thinks
game = BALLOT + [mv, pv[1]]
mv, info, dt = h.go(game)
check('ponder 1: predicted reply is a hit', 'ponder hit' in info and dt < 1.6, f'{mv} {dt:.2f}s [{info}]')
game += [mv]
# a reply that is not the predicted one -> miss
pv = pv_of(info)
time.sleep(0.3)
pp = h.cmd('get ponderpos').split()
h.send('position startpos moves ' + ' '.join(game)); h.expect('ok', 10)
# pick a legal reply different from the predicted one: ask the engine itself (ponder stops on a non-query cmd?
# 'position' is not an enginecommand, so the ponder search still runs) -> use pv alternatives via cbhost error check
alt = None
for cand in ['9-13', '9-14', '10-14', '10-15', '12-16', '7-11', '6-10', '5-9', '1-5', '2-6', '3-7', '4-8',
             '21-17', '22-17', '22-18', '23-18', '24-20', '24-19', '25-21', '26-22', '27-23', '28-24', '27-24',
             '19-16', '19-15', '18-14', '18-15', '17-13', '17-14', '20-16', '15-18', '15-19', '16-20', '16-19',
             '11-16', '8-12', '15x24', '19x10', '18x11']:
    if len(pv) > 1 and cand == pv[1]:
        continue
    h.send('position startpos moves ' + ' '.join(game + [cand]))
    if h.expect('', 10)[0] == 'ok':
        alt = cand
        break
mv, info, dt = h.go(game + [alt])
check('ponder 1: other reply is a miss, answered in time', 'ponder miss' in info and dt < 1.6,
      f'reply {alt} (predicted {pv[1] if len(pv) > 1 else "?"}): {mv} {dt:.2f}s [{info}]')
# game end while pondering: new game at once
time.sleep(0.2)
pp = h.cmd('get ponderpos')
mv, info, dt = h.go(['9-14', '22-18', '5-9'], reset=True, newgame=True)
check('ponder 1: new game while pondering', dt < 1.6 and 'ponder miss' in info, f'[{pp}] -> {mv} {dt:.2f}s [{info}]')
# ten fast games-ends: new game straight after each move
worst = 0
for i in range(10):
    mv, info, dt = h.go(BALLOT, t=0.1, reset=True, newgame=True)
    worst = max(worst, dt)
check('ponder 1: 10 new games right after a move', worst < 0.8, f'worst {worst:.2f}s')
# settings while pondering (stop first)
mv, info, dt = h.go(BALLOT, reset=True, newgame=True)
t0 = time.time(); r = h.cmd('set hashsize 512'); dts = time.time() - t0
check('ponder 1: enginecommand while pondering stops it', dts < 1.0 and h.cmd('get ponderpos').strip() == 'reply 1',
      f'{dts:.2f}s')
mv, info, dt = h.go(BALLOT + [mv, '22-18'] if False else BALLOT, reset=True, newgame=True)
time.sleep(0.2)
t0 = time.time(); h.send('quit'); h.p.wait(15)
check('ponder 1: quit while pondering', time.time() - t0 < 5, f'{time.time() - t0:.2f}s rc {h.p.returncode}')
# EOF while pondering (the match runner closing the pipe)
h = Host(['set hashsize 512', 'set threads 4', 'set book 0', 'set ponder 1'])
mv, info, dt = h.go(BALLOT, reset=True, newgame=True)
time.sleep(0.2)
t0 = time.time(); h.p.stdin.close(); h.p.wait(15)
check('ponder 1: EOF while pondering', time.time() - t0 < 5, f'{time.time() - t0:.2f}s rc {h.p.returncode}')

print('ALL PASSED' if not fails else 'FAILED: ' + ', '.join(fails))
sys.exit(1 if fails else 0)
