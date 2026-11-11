from pathlib import Path
import csv,json,os
b=Path(__file__).parent;d=b/'wins';out=d/'searches';out.mkdir(exist_ok=True)
games=json.loads((b/'wins.json').read_text());roots={r['id']:r for r in csv.DictReader((d/'roots.tsv').open(),delimiter='\t')}
allrows=list(csv.DictReader((d/'occurrences.tsv').open(),delimiter='\t'));children={(r['game'],r['ply']):r for r in csv.DictReader((d/'db-children.tsv').open(),delimiter='\t')};pending={};states={}
def result(r,budget,move=None,depth=127):
 mode='root' if move is None else 'force';token='all' if move is None else move.replace('x','_x_').replace('-','_')
 p=out/f'{r}-n6c-{budget}-{mode}-{depth}-{token}.tsv'
 if p.exists():
  rows=list(csv.DictReader(p.open(),delimiter='\t'))
  if len(rows)==1:return rows[0]
 if move is not None:
  for tracefile in out.glob(f'{r}-n6c-{budget}-force-*-{token}.tsv.iterations.tsv'):
   for step in csv.DictReader(tracefile.open(),delimiter='\t'):
    if int(step['depth'])==depth:
     row=dict(id=r,net='n6c',mode=mode,budget=budget,target=depth,depth=depth,score=step['score'],actual_score=step['score'],partial='0',move=step['move'],actual_move=step['move'],nodes=step['nodes'],ms='',tbhits='',static='',pv=step['pv'],derived_from=str(tracefile))
     with p.open('w') as f:
      writer=csv.DictWriter(f,fieldnames=list(row),delimiter='\t',lineterminator='\n');writer.writeheader();writer.writerow(row)
     return row
 pending[str(p)]=dict(id=r,net='n6c',budget=budget,mode=mode,move=move or '-',depth=depth,path=str(p));case_pending[str(p)]=pending[str(p)];return None
def pair(r,played,alt,dep):
 for _ in range(10):
  x=result(r,100000000,played,dep);y=result(r,100000000,alt,dep)
  if x is None or y is None:return None
  k=min(int(x['depth']),int(y['depth']))
  if k==dep:return dict(depth=k,played_score=int(x['score']),alt_score=int(y['score']),gain=int(y['score'])-int(x['score']),alternative=alt)
  assert k>0;dep=k
 raise AssertionError('no common depth')
def cls(s):return 1 if s>=15000 else -1 if s<=-15000 else 0
def clear(p):
 if cls(p['played_score']) or cls(p['alt_score']):return cls(p['alt_score'])>cls(p['played_score'])
 return p['gain']>30
for g in games:
 trace=[];answer=None;stop=None;case_pending={};rows=[r for r in allrows if r['game']==g['id']]
 for row in rows:
  if row['db']=='3':
   stop=dict(id=row['id'],ply=int(row['ply'])+1,fen=roots[row['id']]['fen'],status='board database already a loss before this move');break
  if row['db'] in ('1','2') and (row['game'],row['ply']) in children:
   ch=children[row['game'],row['ply']]['child_db']
   if (row['db']=='1' and ch=='3') or (row['db']=='2' and ch=='2'):continue
  if int(row['legal'])<2:continue
  rid=row['id'];played=row['played'];a=result(rid,20000000)
  if a is None:break
  t=dict(id=rid,ply=int(row['ply'])+1,pieces=int(row['pieces']),fen=roots[rid]['fen'],played=played,move20=a['actual_move'],root20=int(a['score']),depth20=int(a['depth']),completed_move20=a['move'])
  if int(a['score'])<=-15000:
   h=result(rid,100000000)
   if h is None:break
   t.update(move100=h['actual_move'],root100=int(h['score']),depth100=int(h['depth']),completed_move100=h['move'])
   if int(h['score'])<=-15000:t['status']='root already reports a proven loss at both budgets';trace.append(t);stop=t;break
  if a['actual_move']==played:t['status']='same20';trace.append(t);continue
  p=pair(rid,played,a['actual_move'],int(a['depth']))
  if p is None:break
  t['pair20']=p
  if not clear(p):t['status']='20M pair not clearly worse';trace.append(t);continue
  h=result(rid,100000000)
  if h is None:break
  t.update(move100=h['actual_move'],root100=int(h['score']),depth100=int(h['depth']),completed_move100=h['move'])
  if h['actual_move']==played:t['status']='100M plays recorded move';trace.append(t);continue
  if h['actual_move']!=a['actual_move']:
   p=pair(rid,played,h['actual_move'],int(a['depth']))
   if p is None:break
   t['pair20']=p
  q=pair(rid,played,h['actual_move'],int(h['depth']))
  if q is None:break
  t['pair100']=q
  if clear(p) and clear(q):t['status']='clear shared rejection';trace.append(t);answer=t;break
  t['status']='100M pair not clearly worse';trace.append(t)
 jobs=list(case_pending.values())
 done=answer is not None or stop is not None or not jobs
 states[g['id']]=dict(game=g['id'],trace=trace,answer=answer,stop=stop,done=done,next=jobs[:1])
state=dict(cases=states,done=all(s['done'] for s in states.values()),finished=sum(s['done'] for s in states.values()))
waiting=[s for s in states.values() if not s['done']];by_id={g['id']:g for g in games};waiting.sort(key=lambda s:(by_id[s['game']]['run'].startswith('bl-t1a-60-cake-H3'),len(s['trace']),int(s['game'][1:])))
state['next']=next((s['next'] for s in waiting if s['next']),[])
temp=d/('state.'+str(os.getpid())+'.tmp');temp.write_text(json.dumps(state,indent=2));temp.replace(d/'state.json');print(json.dumps(dict(done=state['done'],finished=state['finished'],total=len(states),next=state['next'])))
