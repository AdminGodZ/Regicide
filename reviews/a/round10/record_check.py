import pathlib,re,gzip,csv,collections,subprocess,hashlib,json
b=pathlib.Path(__file__).resolve().parent
exec((b/'prepare.py').read_text().split('records=[];sources=[]')[0])
def write(name,rows,fields=None):
 with open(b/'tables'/name,'w')as f:
  w=csv.DictWriter(f,fieldnames=fields or list(rows[0]),delimiter='\t',lineterminator='\n');w.writeheader();w.writerows(rows)
def wdl(res,own):return 'U'if res=='*'else'D'if res=='1/2-1/2'else'W'if(res=='1-0')==(own=='B')else'L'
ballots={tuple(m.split()):n for n,m in(line.split('\t',1)for line in (b/'ballots.tsv').read_text().splitlines())}
runs=['v11f-c6-fresh','v11f-c6-kept','v11f-c6-natural','mo3-cake','mo3-kingsrow','mo3-guinn','w14c6-kingsrow-fresh','w14c6-kingsrow-kept'];games=[];sources=[]
for run in runs:
 d=b/'booklab-src/experiments/booklab/tests'/run;rows=list(csv.DictReader(open(d/'results.csv')));by={int(r['game']):r for r in rows};seen=set()
 raw=gzip.open(d/'games.pdn.gz','rt').read()
 for g in re.split(r'(?=\[Event )',raw):
  if not g.strip():continue
  tags,ms,cs=parse(g);pair,leg=map(int,tags['Round'].split('.'));gn=2*(pair-1)+leg
  assert gn not in seen;seen.add(gn);row=by[gn];own='B'if tags['Black'].startswith('Regicide')else'W'
  assert len(ms)==int(row['engine_plies'])+3 and tags['Result']==row['result']
  assert float(row['score_a'])=={'W':1,'D':.5,'L':0}[wdl(tags['Result'],own)]if tags['Result']!='*'else row['score_a']==''
  if run.startswith('mo3-'):assert tags['Worker']=='0'and tags['EngineGamesA']==tags['EngineGamesB']==str(gn-1)
  games.append(dict(id=run+':'+str(gn),run=run,game=gn,tags=tags,moves=ms,comments=cs,own=own,csv=row))
 assert seen==set(by);sources.append(dict(run=run,games=len(rows),sha256=hashlib.sha256(raw.encode()).hexdigest()))
inp=''.join(g['id']+'\t'+g['tags'].get('FEN','')+'\t'+' '.join(g['moves'])+'\n'for g in games)
p=subprocess.run(['nice','-n','19',str(b/'replay_records')],input=inp,text=True,capture_output=True,check=True,timeout=180)
assert 'ERR\t'not in p.stdout
traces=collections.defaultdict(list)
for l in p.stdout.splitlines():
 if l.startswith('END'):continue
 f=l.split('\t');assert len(f)==11;traces[f[0]].append(f)
out=[];details=[];c6=[];summ=[]
for g in games:
 t=g['tags'];tr=traces[g['id']];assert len(tr)==len(g['moves']);claims=[];counts=[[0,0,0],[0,0,0]]
 g['opening']=ballots[tuple(x[3]for x in tr[:3])]
 for i,(x,c)in enumerate(zip(tr,g['comments']),1):
  kinds=re.findall(r'\bclaim=(win|loss|draw)\b',c);assert len(kinds)<=1
  if kinds:
   kind=kinds[0];actor='Regicide'if x[2]==g['own']else t['White'if x[2]=='W'else'Black'];claims.append((i,x[2],kind,actor));counts[0 if actor=='Regicide'else 1][['win','loss','draw'].index(kind)]+=1
 for k,v in zip(['ClaimsA','ClaimsB'],counts):assert t[k]==f'w{v[0]} l{v[1]} d{v[2]}'
 fc=claims[0]if claims else None;fct=f'ply {fc[0]} {fc[1]} {fc[2]}'if fc else'none'
 assert fct==t['FirstClaim']==g['csv']['first_claim']
 cat=g['csv']['termination'];actor=side=kind='-';end='board'
 if cat in ['draw claim','win claim','loss claim']:
  assert g['run'].startswith('mo3-')and fc and fc[0]==len(tr)and tr[-1][9]=='-'
  side,kind,actor=fc[1:]
  implied='1/2-1/2'if kind=='draw'else'1-0'if(side=='B')==(kind=='win')else'0-1';assert implied==t['Result'];end='claim'
 elif cat in ['repetition','no progress','no legal moves']:
  assert tr[-1][9]==t['Result'];assert not any(x[9]!='-'for x in tr[3:-1])
 elif cat=='move limit':assert t['Result']=='*';end='limit'
 else:raise RuntimeError((g['id'],cat))
 r=dict(run=g['run'],game=g['game'],round=t['Round'],opening=g['opening'],our_colour=g['own'],result=t['Result'],WDL=wdl(t['Result'],g['own']),Worker=t['Worker'],EngineGamesA=t['EngineGamesA'],EngineGamesB=t['EngineGamesB'],end=end,termination=cat,claim_actor=actor,claim_kind=kind,claim_side=side,total_plies=len(tr),first_claim=fct,board_result_after_last=tr[-1][9],final_fen=tr[-1][4])
 out.append(r);g['trace']=tr
 if g['opening'].startswith('ACF #6 ')and g['own']=='B':
  searched=[]
  for i,(x,c)in enumerate(zip(tr,g['comments']),1):
   if x[2]!=g['own']:continue
   ns=re.search(r'\bnodes\s+(\d+)',c);sc=re.search(r'\bscore\s+(-?\d+)',c);dep=re.search(r'\bdepth\s+(\d+)',c)
   if ns and int(ns[1])>0:searched.append((i,x[3],int(sc[1]),int(ns[1]),int(dep[1])))
  first=searched[0]if searched else('-',)*5
  c6.append(dict(run=g['run'],game=g['game'],round=t['Round'],Worker=t['Worker'],EngineGamesA=t['EngineGamesA'],EngineGamesB=t['EngineGamesB'],first_search_ply=first[0],first_search_move=first[1],first_search_score=first[2],nodes=first[3],depth=first[4],line=' '.join(x[3]for x in tr)))
for run in runs:
 rs=[r for r in out if r['run']==run];cnt=collections.Counter(r['WDL']for r in rs);ends=collections.Counter((r['termination'],r['claim_actor'],r['claim_kind'])for r in rs)
 summ.append(dict(run=run,games=len(rs),W=cnt['W'],D=cnt['D'],L=cnt['L'],U=cnt['U']))
 for(k,a,c),n in sorted(ends.items()):details.append(dict(run=run,termination=k,claim_actor=a,claim_kind=c,games=n))
write('record-games.tsv',out);write('record-summary.tsv',summ);write('record-endings.tsv',details);write('decisive-ages.tsv',[r for r in out if r['WDL']!='D']);write('acf6-black.tsv',c6);write('validation-sources.tsv',sources)
(b/'validation-games.json').write_text(json.dumps([{k:v for k,v in g.items()if k!='trace'}for g in games]))
for r in summ:print(r)
for r in details:
 if r['run'].startswith('mo3-'):print(r)
print('replayed games',len(games),'moves',sum(len(g['moves'])for g in games))
print('ACF6 Black mo3',[(r['run'],r['game'],r['first_search_ply'],r['first_search_move'],r['first_search_score'])for r in c6 if r['run'].startswith('mo3-')])
