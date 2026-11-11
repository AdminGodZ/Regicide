import pathlib,gzip,re,csv,json,collections
b=pathlib.Path(__file__).resolve().parent;t=b/'tables';cache=json.loads((b.parent/'round6/record-cache.json').read_text())
strict=[g for g in cache if g['opponent'] in ['cake','kingsrow'] and g['run'].startswith(('mo-','mo1-'))]
assert len(strict)==1872
def parse(raw):
 tags=dict(re.findall(r'^\[(\w+) "([^"]*)"\]\s*$',raw,re.M));body=re.sub(r'^\[[^\n]*\]\s*','',raw,flags=re.M);moves=[];comments=[];variation=0
 for tok in re.findall(r'\{[^}]*\}|\(|\)|\S+',body):
  if tok=='(':variation+=1
  elif tok==')':variation=max(0,variation-1)
  elif tok.startswith('{'):
   if not variation and moves:comments[-1]+=tok[1:-1]
  elif not variation and tok not in ['1-0','0-1'] and re.fullmatch(r'\d+(?:[-x:]\d+)+',tok):moves.append(tok);comments.append('')
 return tags,moves,comments
parsed={}
for run in sorted({g['run'] for g in strict}):
 p=b/'booklab-src/experiments/booklab/tests'/run/'games.pdn.gz'
 for raw in re.split(r'(?=\[Event )',gzip.open(p,'rt').read()):
  if raw.strip():
   tags,m,c=parse(raw);key=(run,tags['Round']);assert key not in parsed;parsed[key]=(m,c)
events={}
for p in (b.parent/'round6/tables').glob('first-varying-move-games-[12].tsv'):
 for e in csv.DictReader(open(p),delimiter='\t'):events[(e['run'],e['round'])]=e
contexts=collections.defaultdict(list)
for g in strict:contexts[(g['opponent'],g['opening'],g['our_colour'],g['start'])].append(g)
def write(n,rows):
 with (t/n).open('w',newline='') as f:w=csv.DictWriter(f,fieldnames=list(rows[0]),delimiter='\t',lineterminator='\n');w.writeheader();w.writerows(rows)
def peer_departure(g):
 peers=[p for p in contexts[(g['opponent'],g['opening'],g['our_colour'],g['start'])] if p['id']!=g['id']]
 for i,cur in enumerate(g['canonical']):
  refs={p['canonical'][i][1]:p['canonical'][i][0] for p in peers if len(p['canonical'])>i}
  if cur[1] not in refs:return dict(strict_departure_ply=i+1,strict_departure_actor='Regicide' if cur[2][0]==g['our_colour'] else g['opponent'],strict_departure_move=cur[0],strict_peer_moves=','.join(sorted(refs.values())) or 'peers ended',strict_departure_fen=cur[2])
  peers=[p for p in peers if len(p['canonical'])>i and p['canonical'][i][1]==cur[1]]
 return dict(strict_departure_ply='-',strict_departure_actor='identical',strict_departure_move='-',strict_peer_moves='-',strict_departure_fen='-')
selected=[];control=[];quality=collections.Counter();allrows=[]
for g in strict:
 m,cs=parsed[(g['run'],g['round'])];assert len(m)==len(g['canonical']);own=[]
 for i,(mv,sig,fen) in enumerate(g['canonical']):
  if i<3 or fen[0]!=g['our_colour']:continue
  c=cs[i];nm=re.search(r'\bnodes\s+(\d+)\b',c);sc=re.search(r'\bscore\s+([+-]?\d+)\b',c)
  if not nm:quality['own_missing_nodes']+=1
  elif int(nm[1])==0:own.append((i+1,'zero',int(sc[1]) if sc else None))
  else:
   own.append((i+1,'searched',int(sc[1]) if sc else None))
   if not sc:quality['own_searched_missing_score']+=1
 searched=[v for v in own if v[1]=='searched' and v[2] is not None]
 if not searched:quality['no_own_scored_search']+=1;continue
 first=searched[0];lowest=min(searched,key=lambda x:(x[2],x[0]));initial_zero=[v[0] for v in own if v[0]<first[0] and v[1]=='zero'];firstlow=first[2]<=-40;laterlow=any(v[2]<=-60 for v in searched[1:])
 pair,leg=map(int,g['round'].split('.'));e=events.get((g['run'],g['round']),{})
 row=dict(opponent=g['opponent'],opening=g['opening'],our_colour=g['our_colour'],run=g['run'],round=g['round'],game=2*(pair-1)+leg,worker=g['worker'],EngineGamesA=g['age_A'],EngineGamesB=g['age_B'],result=g['result'],first_searched_ply=first[0],first_searched_score=first[2],book_end_before_ply=first[0],previous_opponent_ply=first[0]-1,last_initial_zero_node_ply=max(initial_zero) if initial_zero else '-',lowest_score=lowest[2],lowest_score_ply=lowest[0],lowest_score_fen=g['canonical'][lowest[0]-1][2],criterion='first<=-40 and later<=-60' if firstlow and laterlow else 'first<=-40' if firstlow else 'later<=-60' if laterlow else '-',first_varying_opponent_ply=e.get('ply','-'),first_varying_opponent_move=e.get('choice','-'),first_varying_opponent_fen=e.get('fen','-'))
 row.update(peer_departure(g));allrows.append(row)
 if g['result']=='D':
  quality['drawn_games']+=1
  if firstlow or laterlow:selected.append(row)
 if g['run']=='mo1-kingsrow' and g['round']=='5.1':control.append(row)
selected.sort(key=lambda r:(r['lowest_score'],r['lowest_score_ply'],r['opponent'],r['opening'],r['our_colour'],r['run'],r['game']))
write('near-miss-games.tsv',selected);write('known-loss-control.tsv',control)
assert control[0]['first_varying_opponent_ply']=='50' and control[0]['strict_departure_ply']==50 and control[0]['strict_departure_actor']=='kingsrow'
summary=[]
for run in sorted({g['run'] for g in strict}):
 ss=[r for r in selected if r['run']==run];ds=[g for g in strict if g['run']==run and g['result']=='D'];summary.append(dict(run=run,draws=len(ds),near_misses=len(ss),minimum_score=min((r['lowest_score'] for r in ss),default='-')))
write('near-miss-summary.tsv',summary)
print('quality',dict(quality),'strict games',len(strict),'selected',len(selected))
print('opponents',collections.Counter(r['opponent'] for r in selected),'first criteria',sum('first<=' in r['criterion'] for r in selected),'later criteria',sum('later<=' in r['criterion'] for r in selected))
print('strict first departures',collections.Counter(r['strict_departure_actor'] for r in selected))
for r in summary:print(r)
print('LOWEST 15')
for r in selected[:15]:print({k:r[k] for k in ['opponent','opening','our_colour','run','game','EngineGamesA','EngineGamesB','first_searched_ply','first_searched_score','lowest_score','lowest_score_ply','first_varying_opponent_ply','first_varying_opponent_move']})
print('LOSS CONTROL',control)
