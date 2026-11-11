import pathlib,csv,gzip,re,subprocess,collections,json,statistics,hashlib
b=pathlib.Path(__file__).resolve().parent;t=b/'tables';src=b/'booklab-src/experiments/booklab/tests'
def parse(raw):
 tags=dict(re.findall(r'^\[(\w+) "([^"]*)"\]\s*$',raw,re.M));body=re.sub(r'^\[[^\n]*\]\s*','',raw,flags=re.M);moves=[];comments=[]
 for tok in re.findall(r'\{[^}]*\}|\S+',body):
  if tok.startswith('{'):
   if moves:comments[-1]+=tok[1:-1]
  elif tok not in ['1-0','0-1'] and re.fullmatch(r'\d+(?:[-x:]\d+)+',tok):moves.append(tok);comments.append('')
 return tags,moves,comments
def write(name,rows,fields=None):
 with open(t/name,'w')as f:
  w=csv.DictWriter(f,fieldnames=fields or list(rows[0]),delimiter='\t',lineterminator='\n');w.writeheader();w.writerows(rows)
def result(pdn,our):
 return 'D' if pdn=='1/2-1/2' else 'W' if (pdn=='1-0')==(our=='B') else 'L'
def implied(kind,side):
 return '1/2-1/2' if kind=='draw' else '1-0' if (side=='B')==(kind=='win') else '0-1'
def band(p):
 return '1-20' if p<=20 else '21-40' if p<=40 else '41-60' if p<=60 else '61-80' if p<=80 else '81-100' if p<=100 else '101+'
ballots={tuple(moves.split()):name for name,moves in (line.split('\t',1) for line in (b/'ballots.tsv').read_text().splitlines())}
games=[];sources=[]
for path in sorted(src.glob('cl-*/games.pdn.gz')):
 run=path.parent.name;opp=run.split('-')[1];raw=gzip.open(path,'rt').read();csvrows=list(csv.DictReader(open(path.parent/'results.csv')));cr={int(x['game']):x for x in csvrows};assert len(cr)==len(csvrows)==156
 n=0
 for rawg in re.split(r'(?=\[Event )',raw):
  if not rawg.strip():continue
  tags,moves,comments=parse(rawg);pair,leg=map(int,tags['Round'].split('.'));game=2*(pair-1)+leg;row=cr[game]
  assert tags['Result']==row['result'];assert len(moves)==int(row['engine_plies'])+3
  own='B'if tags['Black'].startswith('Regicide')else'W';assert tags['White'if own=='W'else'Black'].startswith('Regicide')
  games.append(dict(id=run+':'+str(game),run=run,opponent=opp,tags=tags,moves=moves,comments=comments,csv=row,game=game,own=own))
  n+=1
 assert n==156
 sources.append(dict(run=run,games=n,sha256=hashlib.sha256(gzip.open(path,'rb').read()).hexdigest()))
assert len(games)==936
inp=''.join(g['id']+'\t'+g['tags'].get('FEN','')+'\t'+' '.join(g['moves'])+'\n'for g in games)
replay=subprocess.run(['nice','-n','19',str(b/'census_referee')],input=inp,text=True,capture_output=True,check=True,timeout=60)
assert 'ERR\t'not in replay.stdout,replay.stdout
traces=collections.defaultdict(list)
for line in replay.stdout.splitlines():
 if line.startswith('END'):continue
 x=line.split('\t');assert len(x)==10;traces[x[0]].append(x)
allrows=[];events=[];gamekind=collections.Counter();eventkind=collections.Counter();firstkind=collections.Counter();hist=collections.Counter()
for g in games:
 tr=traces[g['id']];assert len(tr)==len(g['moves']);tags=g['tags'];our=g['own'];opp=g['opponent'];claimlist=[];counts=[[0,0,0],[0,0,0]];kinds=['win','loss','draw'];seen=set()
 for i,(x,c)in enumerate(zip(tr,g['comments'])):
  matches=re.findall(r'\bclaim=(win|loss|draw)\b',c);assert len(matches)<=1
  if not matches:continue
  kind=matches[0];side=x[2];actor='Regicide'if side==our else opp;ix=0 if actor=='Regicide'else 1;counts[ix][kinds.index(kind)]+=1
  claimlist.append((i+1,side,kind,actor))
  key=(opp,actor,side,kind);seen.add(key);eventkind[key]+=1
  events.append(dict(opponent=opp,run=g['run'],game=g['game'],round=tags['Round'],actor=actor,side=side,kind=kind,ply=i+1))
 for key in seen:gamekind[key]+=1
 for k,vals in zip(['ClaimsA','ClaimsB'],counts):assert tags[k]==f'w{vals[0]} l{vals[1]} d{vals[2]}',(g['id'],tags[k],vals)
 first=claimlist[0]if claimlist else None
 firsttxt=f'ply {first[0]} {first[1]} {first[2]}'if first else 'none'
 assert firsttxt==tags['FirstClaim']==g['csv']['first_claim'],g['id']
 if first:
  p,side,kind,actor=first;firstkind[(opp,actor,side,kind)]+=1;hist[(opp,actor,side,kind,p)]+=1
  firstimplied=implied(kind,side);rootfen=tr[p-2][4]if p>1 else tags.get('FEN','startpos')
  rootrev=int(tr[p-2][7])if p>1 else 0
  firstpieces=len(re.findall(r'\d+',rootfen))if rootfen!='startpos'else 24
 else:p=0;side=kind=actor='-';firstimplied='-';rootfen='-';rootrev=firstpieces='-'
 # Referee comes before the claim on the same move. No move limit in this requested counterfactual.
 cf=None;cfply=0;cfwhy=''
 for i,x in enumerate(tr,1):
  if i<=3:continue
  if x[9]!='-':cf=x[9];cfply=i;cfwhy='board';break
  if first and i==p:cf=firstimplied;cfply=i;cfwhy='claim';break
 assert cf is not None,(g['id'],'no counterfactual termination')
 assert tr[-1][9]==tags['Result'],(g['id'],tr[-1],tags['Result'])
 opfirst=next((c for c in claimlist if c[3]!= 'Regicide'),None);ourfirst=next((c for c in claimlist if c[3]=='Regicide'),None)
 row=dict(opponent=opp,run=g['run'],round=tags['Round'],game=g['game'],opening=ballots[tuple(x[3]for x in tr[:3])],our_colour=our,worker=tags['Worker'],EngineGamesA=tags['EngineGamesA'],EngineGamesB=tags['EngineGamesB'],board_result=tags['Result'],board_WDL=result(tags['Result'],our),total_plies=len(tr),first_claim=firsttxt,first_claim_actor=actor,first_claim_ply=p or '-',first_claim_root_fen=rootfen,first_claim_root_rev=rootrev,first_claim_root_pieces=firstpieces,first_claim_implied=firstimplied,first_disagrees=int(bool(first)and firstimplied!=tags['Result']),cb_claims_result=cf,cb_claims_WDL=result(cf,our),cb_claims_end_ply=cfply,cb_claims_reason=cfwhy,opponent_first_claim=f'ply {opfirst[0]} {opfirst[2]}'if opfirst else 'none',our_first_claim=f'ply {ourfirst[0]} {ourfirst[2]}'if ourfirst else 'none',opponent_any_draw=int(any(c[3]!='Regicide'and c[2]=='draw'for c in claimlist)))
 allrows.append(row)
write('census-games.tsv',allrows)
write('census-sources.tsv',sources)
kindrows=[dict(opponent=k[0],actor=k[1],side=k[2],kind=k[3],games=v,events=eventkind[k],first_claims=firstkind[k])for k,v in sorted(gamekind.items())]
write('census-side-kind.tsv',kindrows)
hrows=[dict(opponent=k[0],actor=k[1],side=k[2],kind=k[3],first_claim_ply=k[4],games=v)for k,v in sorted(hist.items())]
write('census-first-ply.tsv',hrows)
focus=[r for r in allrows if r['first_disagrees']or r['board_WDL']!='D'];write('census-decisions.tsv',focus)
mismatches=[r for r in allrows if r['first_disagrees']];write('census-disagreements.tsv',mismatches,list(allrows[0]))
summary=[]
for opp in ['cake','kingsrow','guinn']:
 rs=[r for r in allrows if r['opponent']==opp];ps=[int(r['first_claim_ply'])for r in rs if r['first_claim_ply']!='-'];wdl=collections.Counter(r['board_WDL']for r in rs);cw=collections.Counter(r['cb_claims_WDL']for r in rs)
 summary.append(dict(opponent=opp,games=len(rs),claim_games=len(ps),no_claim_games=len(rs)-len(ps),first_min=min(ps),first_median=statistics.median(ps),first_max=max(ps),first_1_20=sum(p<=20 for p in ps),first_21_40=sum(21<=p<=40 for p in ps),first_41_60=sum(41<=p<=60 for p in ps),first_61_80=sum(61<=p<=80 for p in ps),first_81_100=sum(81<=p<=100 for p in ps),first_101_plus=sum(p>100 for p in ps),board_W=wdl['W'],board_D=wdl['D'],board_L=wdl['L'],cb_W=cw['W'],cb_D=cw['D'],cb_L=cw['L'],first_disagrees=sum(r['first_disagrees']for r in rs)))
write('census-summary.tsv',summary)
for r in summary:print(r)
print('side/kind')
for r in kindrows:print(r)
print('focus games',len(focus),'mismatches',len(mismatches),'moves independently replayed',sum(len(g['moves'])for g in games),'decisive opponent draw claims',sum(r['opponent_any_draw']for r in focus))
