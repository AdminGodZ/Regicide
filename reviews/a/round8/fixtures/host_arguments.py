from pathlib import Path
import subprocess
b=Path('reports/round8').resolve();dll=Path('reports/round5/mock_engine.so').resolve()
log=b/'evidence/host-arguments.log'
if log.exists():log.unlink()
p=subprocess.Popen(['nice','-n','19',str(b/'claim-src/build/tools/cbhost'),str(dll)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,bufsize=1)
def wait(want):
 while True:
  x=p.stdout.readline().strip()
  if not x:raise RuntimeError('host closed')
  if x.startswith(want):return x
def send(s,want):p.stdin.write(s+'\n');p.stdin.flush();return wait(want)
wait('ready');send('cmd set log '+str(log),'reply');send('newgame','ok')
for fen,time in [('B:WK32:BK1','.5'),('B:WK28:BK5','.66'),('B:WK32:BK1','.82')]:
 send('position fen '+fen,'ok');send('go maxtime '+time,'bestmove')
p.stdin.write('quit\n');p.stdin.flush();p.communicate(timeout=10);assert p.returncode==0
print(log.read_text())
