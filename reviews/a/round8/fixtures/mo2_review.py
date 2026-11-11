from pathlib import Path
import csv,gzip,re,collections,subprocess,hashlib
b=Path(__file__).resolve().parent;t=b/'tables';src=b/'booklab-src/experiments/booklab/tests'
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
def wdl(pdn,our):
 return 'U'if pdn=='*'else'D'if pdn=='1/2-1/2'else'W'if(pdn=='1-0')==(our=='B')else'L'
def implied(kind,side):return '1/2-1/2'if kind=='draw'else'1-0'if(side=='B')==(kind=='win')else'0-1'
ballots={tuple(m.split()):n for n,m in(line.split('\t',1)for line in (b/'ballots.tsv').read_text().splitlines())}
games=[];sources=[]
for path in sorted(src.glob('*/games.pdn.gz')):
 run=path.parent.name
 if not run.startswith(('mo2-','cl-')):continue
 cr=list(csv.DictReader(open(path.parent/'results.csv')));bygame={int(r['game']):r for r in cr};n=0;seen=set()
 for raw in re.split(r'(?=\[Event )',gzip.open(path,'rt').read()):
  if not raw.strip():continue
  tags,moves,comments=parse(raw);pair,leg=map(int,tags['Round'].split('.'));gn=2*(pair-1)+leg;assert gn not in seen;seen.add(gn);row=bygame[gn]
  assert len(moves)==int(row['engine_plies'])+3 and tags['Result']==row['result']
  assert sum(tags[k].startswith('Regicide')for k in ['Black','White'])==1
  own='B'if tags['Black'].startswith('Regicide')else'W'
  assert float(row['score_a'])==({'W':1,'D':.5,'L':0}[wdl(tags['Result'],own)])if tags['Result']!='*'else row['score_a']==''
  if run.startswith('mo2-'):assert tags['Worker']=='0'and tags['EngineGamesA']==tags['EngineGamesB']==str(gn-1)
  games.append(dict(id=run+':'+str(gn),run=run,opponent=run.split('-')[1],tags=tags,moves=moves,comments=comments,csv=row,game=gn,own=own))
  n+=1
 assert n==len(cr)==len(bygame)==(312 if run.startswith('mo2-')else 156)and seen==set(bygame)
 sources.append(dict(run=run,games=n,sha256=hashlib.sha256(gzip.open(path,'rb').read()).hexdigest()))
assert len(games)==1872
inp=''.join(g['id']+'\t'+g['tags'].get('FEN','')+'\t'+' '.join(g['moves'])+'\n'for g in games)
p=subprocess.run(['nice','-n','19',str(b/'replay_records')],input=inp,text=True,capture_output=True,check=True,timeout=90);assert 'ERR\t'not in p.stdout
traces=collections.defaultdict(list)
for line in p.stdout.splitlines():
 if not line.startswith('END'):
  x=line.split('\t');assert len(x)==11;traces[x[0]].append(x)
r7=list(csv.DictReader(open(b/'census-reference.tsv'),delimiter='\t'));r7by={(r['run'],int(r['game'])):r for r in r7}
old={};rows=[];stats=collections.Counter()
for g in games:
 tr=traces[g['id']];assert len(tr)==len(g['moves']);g['trace']=tr;tags=g['tags'];our=g['own'];claims=[];cnt=[[0,0,0],[0,0,0]]
 g['opening']=ballots[tuple(x[3]for x in tr[:3])]
 for i,(x,c)in enumerate(zip(tr,g['comments']),1):
  kinds=re.findall(r'\bclaim=(win|loss|draw)\b',c);assert len(kinds)<=1
  if not kinds:continue
  kind=kinds[0];actor='Regicide'if x[2]==our else g['opponent'];claims.append((i,x[2],kind,actor));cnt[0 if actor=='Regicide'else 1][['win','loss','draw'].index(kind)]+=1
 for field,v in zip(['ClaimsA','ClaimsB'],cnt):assert tags[field]==f'w{v[0]} l{v[1]} d{v[2]}'
 fc=claims[0]if claims else None;fct=f'ply {fc[0]} {fc[1]} {fc[2]}'if fc else'none'
 assert fct==tags['FirstClaim']==g['csv']['first_claim']
 if g['run'].startswith('cl-'):
  baseline=r7by[(g['run'],g['game'])];assert baseline['board_result']==tags['Result']and baseline['first_claim']==fct
  assert tr[-1][9]==tags['Result'];g['cf_len']=int(baseline['cb_claims_end_ply']);g['cf_result']=baseline['cb_claims_result']
  key=(g['opponent'],g['opening'],our);assert key not in old;old[key]=g;continue
 assert not any(x[9]!='-'for x in tr[3:-1]),(g['id'],'earlier board result')
 cat=g['csv']['termination'];actor=kind=side='-'
 if cat in ['draw claim','win claim','loss claim']:
  assert fc and fc[0]==len(tr)and tr[-1][9]=='-',(g['id'],cat,fc,tr[-1])
  actor,kind,side=fc[3],fc[2],fc[1];assert implied(kind,side)==tags['Result']
  assert cat==kind+' claim';end='claim'
 elif cat in ['repetition','no progress','no legal moves']:
  assert tr[-1][9]==tags['Result'],(g['id'],cat,tr[-1]);end='board'
  if cat=='repetition':assert int(tr[-1][8])>=3
  if cat=='no progress':assert int(tr[-1][7])>=79
  if cat=='no legal moves':assert int(tr[-1][6])==0
 elif cat=='move limit':
  assert tags['Result']=='*';end='limit'
 else:raise RuntimeError((g['id'],cat))
 row=dict(run=g['run'],opponent=g['opponent'],game=g['game'],round=tags['Round'],opening=g['opening'],our_colour=our,EngineGamesA=tags['EngineGamesA'],EngineGamesB=tags['EngineGamesB'],worker=tags['Worker'],result=tags['Result'],WDL=wdl(tags['Result'],our),end=end,termination=cat,claim_actor=actor,claim_side=side,claim_kind=kind,total_plies=len(tr),engine_plies=g['csv']['engine_plies'],first_claim=fct,final_fen=tr[-1][4],board_result_after_last=tr[-1][9])
 rows.append(row);g['out']=row;stats[(g['opponent'],end,actor,kind,cat)]+=1
assert len(old)==len(rows)==936
assert len({(r['opponent'],r['opening'],r['our_colour'])for r in rows})==936
def difference(g,c,limit):
 a=g['trace'];d=c['trace'][:limit]
 for i,(x,y)in enumerate(zip(a,d)):
  if x[10]!=y[10]:
   assert (i==0 or a[i-1][4]==d[i-1][4]);return dict(first_diff_ply=i+1,first_diff_actor='Regicide'if x[2]==g['own']else g['opponent'],mo2_move=x[3],census_move=y[3],first_diff_fen=a[i-1][4]if i else g['tags'].get('FEN','startpos'),sequence_relation='different move')
 return dict(first_diff_ply='-',first_diff_actor='-',mo2_move='-',census_move='-',first_diff_fen='-',sequence_relation='identical'if len(a)==len(d)else'mo2 ends first'if len(a)<len(d)else'census ends first')
comparisons=[]
for g in games:
 if not g['run'].startswith('mo2-'):continue
 c=old[(g['opponent'],g['opening'],g['own'])];r=dict(g['out']);r.update(census_run=c['run'],census_game=c['game'],census_cf_result=c['cf_result'],census_cf_WDL=wdl(c['cf_result'],g['own']),census_cf_end_ply=c['cf_len'],census_final_plies=len(c['trace']),census_EngineGamesA=c['tags']['EngineGamesA'],census_EngineGamesB=c['tags']['EngineGamesB'])
 r['result_changed']=int(r['result']!=c['cf_result']);r.update(difference(g,c,c['cf_len']))
 full=difference(g,c,len(c['trace']))
 for k,v in full.items():r['full_'+k]=v
 comparisons.append(r)
rows.sort(key=lambda x:(x['opponent'],x['game']));comparisons.sort(key=lambda x:(x['opponent'],x['game']))
write('mo2-games.tsv',rows);write('mo2-decisive-unknown.tsv',[r for r in rows if r['WDL']!='D'])
write('mo2-comparisons.tsv',comparisons);changed=[r for r in comparisons if r['result_changed']];write('mo2-changed-results.tsv',changed)
write('mo2-sources.tsv',sources)
term=[dict(opponent=k[0],end=k[1],claim_actor=k[2],claim_kind=k[3],termination=k[4],games=v)for k,v in sorted(stats.items())];write('mo2-terminations.tsv',term)
summary=[]
for opp in ['cake','kingsrow','guinn']:
 rs=[r for r in rows if r['opponent']==opp];cs=[r for r in comparisons if r['opponent']==opp];counts=collections.Counter(r['WDL']for r in rs)
 summary.append(dict(opponent=opp,games=len(rs),W=counts['W'],D=counts['D'],L=counts['L'],U=counts['U'],changed_results=sum(r['result_changed']for r in cs),first_diff_Regicide=sum(r['first_diff_actor']=='Regicide'for r in cs),first_diff_opponent=sum(r['first_diff_actor']==opp for r in cs),same_prefix=sum(r['first_diff_actor']=='-'for r in cs)))
write('mo2-summary.tsv',summary)
for r in summary:print(r)
for r in term:print(r)
print('changed results',len(changed),'decisive/unknown',sum(r['WDL']!='D'for r in rows),'new moves replayed',sum(len(g['trace'])for g in games if g['run'].startswith('mo2-')),'all moves replayed',sum(len(g['trace'])for g in games))
for r in changed:print({k:r[k]for k in ['opponent','game','opening','our_colour','census_cf_WDL','WDL','first_diff_ply','first_diff_actor','census_move','mo2_move','sequence_relation']})
print('census no-claim endings',[(g['run'],g['game'],len(g['trace']))for g in old.values()if g['tags']['FirstClaim']=='none'])
lookup={g['id']:g for g in games}
for r in comparisons:
 if r['first_diff_actor']!='Regicide':continue
 i=int(r['first_diff_ply'])-1
 a=lookup[r['run']+':'+str(r['game'])]['comments'][i]
 c=lookup[r['census_run']+':'+str(r['census_game'])]['comments'][i]
 na=re.search(r'\bnodes\s+(\d+)',a);nc=re.search(r'\bnodes\s+(\d+)',c)
 assert na and nc and int(na[1])>0 and int(nc[1])>0,(r,a,c)
print('All 11 first differences by Regicide are searched moves, positive nodes on both sides.')
