from pathlib import Path
import csv,ctypes as C,re
base=Path('reports/round8').resolve()
s=(base/'api_cases.py').read_text()
# Reuse only the board and independent-referee adapter definitions.
s=s[:s.index("for x in ['set hashsize")]
s=s.replace("sys.argv[1]","(sys.argv[1] if len(sys.argv)>1 else str(base/'claim-src/build/libregicide_cb.so'))")
exec(s)
for x in ['set hashsize 1','set threads 1','set book 0','set avoid 0','set evalfile hce']:cmd(x)
assert cmd('get dbpieces')=='0'
h78=(base/'evidence/terminal-history78.txt').read_text().strip().removeprefix('set gamehist ')
t=trace('terminal78',h78);tok=h78.split();h77=t[0][4]+' '+' '.join(tok[2:])
w=csv.writer(__import__('sys').stdout,delimiter='\t',lineterminator='\n');w.writerow(['root_rev','move','CB_return','score','after','referee_result','info'])
for hist in ['B:WK10:BK1,K6',h77,h78]:
 ts=trace('terminal',hist);cmd('set gamehist '+hist);a=board('B:WK10:BK1,K6');info=C.create_string_buffer(1024);play=C.c_int(0);m=C.create_string_buffer(512)
 rc=lib.getmove(a,2,.01,info,C.byref(play),1,0,m);text=info.value.decode();move=re.search(r'\bpv\s+(\S+)',text)[1];score=re.search(r'\bscore\s+(-?\d+)',text)[1];final=trace('terminal',hist,move)[-1]
 w.writerow([ts[-1][7]if ts else 0,move,rc,score,fen(a,'W'),final[-1],text])
