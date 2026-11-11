from pathlib import Path
import re
b=Path(__file__).resolve().parent;mask=(1<<64)-1;seed=0x9E3779B97F4A7C15;psq=[]
for row in range(4):
 out=[]
 for sq in range(32):
  seed=(seed+0x9E3779B97F4A7C15)&mask;z=seed;z=((z^(z>>30))*0xBF58476D1CE4E5B9)&mask;z=((z^(z>>27))*0x94D049BB133111EB)&mask;out.append(z^(z>>31))
 psq.append(out)
want={3174249323745851282,6096625721051765919,11389425374743185192};found={}
for p in (b/'master-src').rglob('*'):
 if not p.is_file()or p.stat().st_size>10000000:continue
 try:s=p.read_text()
 except:continue
 for fen in re.findall(r'[BW]:W(?:K?\d+(?:,K?\d+)*)?:B(?:K?\d+(?:,K?\d+)*)?',s):
  try:
   side,w,bl=fen.split(':');key=0xF1E2D3C4B5A69788 if side=='W'else 0
   for color,part in [(1,w[1:]),(0,bl[1:])]:
    for t in part.split(','):
     if not t:continue
     king=t.startswith('K');n=int(t.lstrip('K'));q=(7-(n-1)//4)*4+(n-1)%4;key^=psq[2*color+king][q]
   if key in want:found[key]=fen
  except:pass
(b/'avoid-fens.txt').write_text('\n'.join(found.values())+'\n');print(found)
