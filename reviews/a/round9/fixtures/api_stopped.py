from pathlib import Path
import sys,ctypes as C
base=Path('reports/round9').resolve();s=(base/'api_cases.py').read_text();s=s[:s.index("for x in ['set hashsize")];exec(s)
for x in ['set hashsize 1','set threads 1','set book 0','set avoid 0','set evalfile hce','set dbmbytes 16','set dbpath '+(base.parent/'round6/db-path.txt').read_text().strip()]:cmd(x)
class Coor(C.Structure):_fields_=[('x',C.c_int),('y',C.c_int)]
class Move(C.Structure):_fields_=[('jumps',C.c_int),('newpiece',C.c_int),('oldpiece',C.c_int),('from_',Coor),('to',Coor),('path',Coor*12),('del_',Coor*12),('delpiece',C.c_int*12)]
p='B:WK30:BK25,K26';cmd('set gamehist '+p);a=board(p);info=C.create_string_buffer(1024);play=C.c_int(1);m=Move()
r=lib.getmove(a,2,.01,info,C.byref(play),1,0,C.byref(m))
n=lambda q:4*q.y+4-q.x//2
print('stop-before-call root',p,'move',str(n(m.from_))+'-'+str(n(m.to)),'return',r,'after',fen(a,'W'),'info',info.value.decode()or'(empty)')
