from pathlib import Path
import csv,json,re,collections,hashlib
b=Path(__file__).resolve().parent
def write(name,rows,fields=None):
 with open(b/'tables'/name,'w')as f:
  w=csv.DictWriter(f,fieldnames=fields or list(rows[0]),delimiter='\t',lineterminator='\n');w.writeheader();w.writerows(rows)
gs=json.loads((b/'records.json').read_text());lookup={}
for g in gs:lookup[(g['run'].removeprefix('bl-'),g['tags'].get('Round'))]=g
ds=list(csv.DictReader(open(b/'booklab-src/experiments/booklab/wp14/dips.tsv'),delimiter='\t'))
ev=[]
for name in ['dip-01.tsv','dip-02-07.tsv','dip-08-13.tsv']:ev+=[x.split('\t')for x in (b/'evidence'/name).read_text().splitlines()]
out=[]
for i,d in enumerate(ds,1):
 src=d['games (run:round)'];run,rd=src.rsplit(':',1);g=lookup[(run,rd)];ply=int(d['ply']);prefix=d['moves_from_start'].split()
 assert prefix==g['moves'][:ply-1]and g['moves'][ply-1]==d['move_played']
 c=g['comments'][ply-1];score=int(re.search(r'\bscore\s+(-?\d+)',c)[1]);depth=int(re.search(r'\bdepth\s+(\d+)',c)[1])
 assert score==int(d['game_score'])and depth==int(d['game_depth'])
 assert abs(score-int(d['score_100m']))>=50
 r=dict(id=i,source=src,opening=d['opening'],colour=d['side_to_move'],ply=ply,FEN=d['fen'],legal_moves=d['legal_moves'],game_score=score,game_depth=depth,score_100M_record=d['score_100m'],history_sha256=hashlib.sha256(d['moves_from_start'].encode()).hexdigest())
 for n in [1000000,3000000,10000000]:
  a=next(x for x in ev if x[0]==f'{i}-{n}');assert a[1]==d['fen']
  for k,j in [('move',5),('score',6),('depth',7),('nodes',8),('partial',9),('completed_score',10)]:r[f'{n//1000000}M_{k}']=a[j]
 out.append(r)
write('dip-results.tsv',out)
a=[x.split('\t')for x in (b/'evidence/draw-before.tsv').read_text().splitlines()];c=[x.split('\t')for x in (b/'evidence/draw-after.tsv').read_text().splitlines()];out=[]
requests={x.split('\t')[0]:x.split('\t')for x in (b/'draw-requests.tsv').read_text().splitlines()}
for x,y in zip(a,c):
 assert x[:5]==y[:5]and x[3]=='2'and x[7]==y[7]=='127'
 gid,ply=map(int,x[0].split(':'));g=gs[gid];assert g['id']==str(gid)
 out.append(dict(id=x[0],run=g['run'],round=g['tags'].get('Round'),ply=ply,FEN=x[1],pieces=sum(len(s[1:].split(','))if s[1:]else 0 for s in x[1].split(':')[1:]),reversible=x[2],before_move=x[5],after_move=y[5],before_score=x[6],after_score=y[6],before_depth=x[7],after_depth=y[7],before_nodes=x[8],after_nodes=y[8],same=int(x[5:7]==y[5:7])))
write('draw-comparison.tsv',out)
# Inputs carry full histories; include requests to reproduce each root.
print('dip score rows verified',len(ds),'stored legal_moves',collections.Counter(d['legal_moves']for d in ds))
print('draw matches',sum(r['same']for r in out),'of',len(out),'colour',collections.Counter(r['FEN'][0]for r in out),'piece counts',collections.Counter(r['pieces']for r in out))
print('draw total nodes',sum(int(x[8])for x in a),sum(int(x[8])for x in c),'ranges',min(int(x[8])for x in a),max(int(x[8])for x in a),min(int(x[8])for x in c),max(int(x[8])for x in c))
