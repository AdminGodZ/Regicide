import ctypes as C,pathlib,sys,subprocess,csv,re
base=pathlib.Path(__file__).resolve().parent; root=base.parent.parent
lib=C.CDLL(str(pathlib.Path(sys.argv[1]).resolve())); boardtype=(C.c_int*8)*8
lib.enginecommand.argtypes=[C.c_char_p,C.c_char_p];lib.getmove.argtypes=[C.POINTER(C.c_int*8),C.c_int,C.c_double,C.c_char_p,C.POINTER(C.c_int),C.c_int,C.c_int,C.c_void_p]
def cmd(x):
 r=C.create_string_buffer(1024);rc=lib.enginecommand(x.encode(),r);assert rc==1,(x,r.value);return r.value.decode()
def board(fen):
 a=boardtype()
 for field in fen.split(':')[1:]:
  col=1 if field[0]=='W' else 2
  for token in field[1:].split(','):
   if not token:continue
   k=token.startswith('K');n=int(token[1:] if k else token)-1;y=n//4;x=2*(3-n%4)+(y&1);a[x][y]=col|(8 if k else 4)
 return a
def fen(a,stm):
 fields=[]
 for c,ch in [(1,'W'),(2,'B')]:
  sq=[]
  for n in range(32):
   y=n//4;x=2*(3-n%4)+(y&1);v=a[x][y]
   if v and v&3==c:sq.append(('K' if v&8 else '')+str(n+1))
  fields.append(ch+','.join(sq))
 return stm+':'+':'.join(fields)
def trace(id,gh,extra=''):
 h=gh.removeprefix('set gamehist ').split()
 r=subprocess.run(['nice','-n','19',str(base/'census_referee')],input=id+'\t'+h[0]+'\t'+' '.join(h[1:])+(' '+extra if extra else '')+'\n',text=True,capture_output=True,check=True)
 assert 'ERR' not in r.stdout,r.stdout
 return [x.split('\t') for x in r.stdout.splitlines() if not x.startswith('END')]
for x in ['set hashsize 1','set threads 1','set book 0','set avoid 0','set evalfile hce','set dbmbytes 16','set dbpath '+(root/'reports/round6/db-path.txt').read_text().strip()]:cmd(x)
S='W:WK2,K8,K12,K18,K20,K28,K29:BK13,K30';P='B:WK2,K8,K12,K18,K20,K28,K29:BK17,K30'
cyc=S+' 28-24 13-17 24-28 17-13 28-24 13-17 24-28'
h78=(root/'reports/round6/evidence/gamehist9-78.txt').read_text().strip().removeprefix('set gamehist ')
t=trace('h78',h78);tokens=h78.split();h77=t[0][4]+' '+' '.join(tokens[2:])
h38='B:WK5,K21:BK6,K7 6-1 5-9 7-10 9-5 1-6 5-1 6-2 21-25 2-6 1-5 6-2 5-9 10-6 9-5 6-1 5-9 1-5 9-14 5-1 14-10 2-6 10-7 1-5 7-3 5-1 3-8 1-5 8-4 5-1 25-30 1-5 4-8 5-1 8-3 1-5 3-7 5-1 7-10'
h38t=trace('h38',h38);ht=h38.split();h37=h38t[0][4]+' '+' '.join(ht[2:])
cases=[
 ('short-black','B:W10:B6','B:W10:B6',''),
 ('short-white','W:W27:B23','W:W27:B23',''),
 ('fresh-table','B:WK3:BK1,K2','B:WK3:BK1,K2',''),
 ('late-table','B:WK3:BK1,K2',(root/'reports/round6/evidence/gamehist77.txt').read_text().strip().removeprefix('set gamehist '),''),
 ('root-already-third',S,cyc+' 17-13',''),
 ('move-makes-third',P,cyc,''),
 ('move-makes-second',P,S+' 28-24 13-17 24-28',''),
 ('reversible-78',P,h78,''),
 ('reversible-77',P,h77,''),
 ('searched-table-4','B:WK10,K30:BK1,K6','B:WK3,K30:BK1,K6 1-5 3-7 5-1 7-10',''),
 ('searched-table-37','B:WK10,K30:BK1,K6',h37,''),
 ('searched-table-38','B:WK10,K30:BK1,K6',h38,''),
 ('future-third','B:WK3:BK1,K2','B:WK8:BK2,K5 5-1 8-3 1-5 3-8 5-1 8-3','3-8'),
]
out=[]
for name,pos,hist,reply in cases:
 ts=trace(name,hist);assert not ts or ts[-1][4]==pos,(name,ts[-1] if ts else '',pos)
 cmd('set gamehist '+hist);a=board(pos);info=C.create_string_buffer(1024);play=C.c_int(0);m=C.create_string_buffer(512)
 claim=lib.getmove(a,1 if pos[0]=='W' else 2,.01,info,C.byref(play),1,0,m)
 text=info.value.decode();match=re.search(r'\bpv\s+(\S+)',text);assert match,(name,text)
 move=match[1];after=fen(a,'B' if pos[0]=='W' else 'W');tr=trace(name,hist,move+(' '+reply if reply else ''))
 assert tr[len(ts)][4]==after,(name,move,after,tr[len(ts)])
 own=tr[len(ts)];last=tr[-1]
 out.append(dict(case=name,root=pos,root_rev=ts[-1][7] if ts else '0',move=move,claim=claim,after=after,referee_after_move=own[-1],after_move_occurrences=own[8],after_move_rev=own[7],opponent_reply=reply or '-',referee_after_reply=last[-1] if reply else '-',info=text))
w=csv.DictWriter(sys.stdout,fieldnames=out[0].keys(),delimiter='\t',lineterminator='\n');w.writeheader();w.writerows(out)
