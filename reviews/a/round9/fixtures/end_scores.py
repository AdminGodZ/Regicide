from pathlib import Path
import gzip,re,csv,collections,hashlib
b=Path(__file__).resolve().parent;t=b/'tables';root=b/'booklab-src/experiments/booklab/tests'
def parse(raw):
 tags=dict(re.findall(r'^\[(\w+) "([^"]*)"\]\s*$',raw,re.M));body=re.sub(r'^\[[^\n]*\]\s*','',raw,flags=re.M);moves=[];comments=[]
 for tok in re.findall(r'\{[^}]*\}|\S+',body):
  if tok.startswith('{'):
   if moves:comments[-1]+=tok[1:-1]
  elif tok not in ['1-0','0-1']and re.fullmatch(r'\d+(?:[-x:]\d+)+',tok):moves.append(tok);comments.append('')
 return tags,moves,comments
def write(name,rows,fields=None):
 with open(t/name,'w')as f:
  w=csv.DictWriter(f,fieldnames=fields or list(rows[0]),delimiter='\t',lineterminator='\n');w.writeheader();w.writerows(rows)
oldsha={r['run']:r['sha256']for r in csv.DictReader(open(b/'round8-sources.tsv'),delimiter='\t')}
games={}
for run,sha in oldsha.items():
 path=root/run/'games.pdn.gz';raw=gzip.open(path,'rb').read();assert hashlib.sha256(raw).hexdigest()==sha
 cr={int(r['game']):r for r in csv.DictReader(open(root/run/'results.csv'))}
 for rawg in re.split(r'(?=\[Event )',raw.decode()):
  if not rawg.strip():continue
  tags,m,c=parse(rawg);a,z=map(int,tags['Round'].split('.'));g=2*(a-1)+z
  assert (run,g)not in games;assert tags['Result']==cr[g]['result']and tags['FirstClaim']==cr[g]['first_claim']
  own='B'if tags['Black'].startswith('Regicide')else'W'
  games[(run,g)]=dict(tags=tags,moves=m,comments=c,own=own,csv=cr[g],run=run,game=g)
assert len(games)==1872
def endpoint(g,end):
 searched=[];reported=[]
 for ply in range(4,end+1):
  side='B'if ply%2 else'W'
  if side!=g['own']:continue
  text=g['comments'][ply-1];s=re.search(r'\bscore\s+(-?\d+)',text);n=re.search(r'\bnodes\s+(\d+)',text)
  assert s and n,(g['run'],g['game'],ply,text)
  reported.append((ply,int(s[1]),int(n[1])))
  if int(n[1])>0:searched.append((ply,int(s[1]),int(n[1])))
 last=searched[-1]if searched else('-','-','-');lastown=reported[-1]if reported else('-','-','-')
 return dict(end_ply=end,last_search_ply=last[0],last_search_score=last[1],last_search_nodes=last[2],last_own_ply=lastown[0],last_own_score=lastown[1],last_own_nodes=lastown[2],searched_moves=len(searched))
changes=list(csv.DictReader(open(b/'round8-changed-results.tsv'),delimiter='\t'));assert len(changes)==24
pairrows=[];detail=[]
for r in changes:
 new=games[(r['run'],int(r['game']))];old=games[(r['census_run'],int(r['census_game']))]
 row={k:r[k]for k in ['opponent','opening','our_colour','run','game','census_run','census_game','census_cf_WDL','WDL']}
 for prefix,g,end in [('mo2',new,len(new['moves'])),('census_claim',old,int(r['census_cf_end_ply'])),('census_played',old,len(old['moves']))]:
  for k,v in endpoint(g,end).items():row[prefix+'_'+k]=v
 pairrows.append(row)
allrows=[];selected=[];eligible=0;missing=0
for (run,gn),g in sorted(games.items()):
 if not run.startswith('mo2-'):continue
 if g['tags']['Result']!='1/2-1/2'or g['csv']['termination']!='draw claim':continue
 fc=g['tags']['FirstClaim'].split();assert fc[3]=='draw'and fc[2]!=g['own']
 assert int(fc[1])==len(g['moves']);eligible+=1
 row=dict(opponent=run.split('-')[1],run=run,game=gn,round=g['tags']['Round'],opening=g['tags']['Opening'].split(': ',1)[-1],our_colour=g['own'],EngineGamesA=g['tags']['EngineGamesA'],EngineGamesB=g['tags']['EngineGamesB'],draw_claim_ply=int(fc[1]))
 # Opening names from the already replayed comparison table are added below.
 row.update(endpoint(g,len(g['moves'])));allrows.append(row)
 if row['last_search_score']=='-':missing+=1
 elif row['last_search_score']>=100:selected.append(row)
bykey={(r['run'],int(r['game'])):r['opening']for r in csv.DictReader(open(b/'round8-games.tsv'),delimiter='\t')}
for r in allrows:r['opening']=bykey[(r['run'],r['game'])]
selected.sort(key=lambda r:(-r['last_search_score'],r['opponent'],r['game']))
write('changed-result-end-scores.tsv',pairrows);write('opponent-draw-claim-end-scores.tsv',allrows);write('draw-claims-plus100.tsv',selected, list(allrows[0]))
summary=[]
for opp in ['cake','kingsrow','guinn']:
 allopp=[r for r in allrows if r['opponent']==opp];ss=[r for r in selected if r['opponent']==opp]
 summary.append(dict(opponent=opp,opponent_draw_claims=len(allopp),no_own_search=sum(r['last_search_score']=='-'for r in allopp),at_least_plus100=len(ss),minimum_last_search=min((r['last_search_score']for r in allopp if r['last_search_score']!='-'),default='-'),maximum_last_search=max((r['last_search_score']for r in allopp if r['last_search_score']!='-'),default='-'),maximum_last_reported=max(r['last_own_score']for r in allopp)))
write('end-score-summary.tsv',summary)
print('input games',len(games),'changed keys',len(pairrows),'opponent draw endings',eligible,'without prior own searched score',missing,'plus100',len(selected))
for r in summary:print(r)
for r in selected:print({k:r[k]for k in ['opponent','game','opening','our_colour','draw_claim_ply','last_search_ply','last_search_score','last_own_ply','last_own_score','last_own_nodes']})
