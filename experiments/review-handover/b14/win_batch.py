from pathlib import Path
import json,csv,subprocess,time
b=Path(__file__).parent;d=b/'wins';start=time.monotonic()
if not (b/'iteration-control.json').exists():
 for tf in (d/'searches').glob('*-force-*.tsv.iterations.tsv'):
  steps=list(csv.DictReader(tf.open(),delimiter='\t'));step=next((x for x in steps if x['depth']=='3'),None)
  if step is None:continue
  main=Path(str(tf).removesuffix('.iterations.tsv'))
  if not main.exists():continue
  src=next(csv.DictReader(main.open(),delimiter='\t'));dest=b/'iteration-control.tsv'
  test=subprocess.run([str(b/'walk'),'force',str(d),src['id'],src['net'],'100000000','3',step['move'],str(dest)],capture_output=True,text=True,timeout=30)
  assert test.returncode==0,test.stderr
  actual=next(csv.DictReader(dest.open(),delimiter='\t'));same=all(str(actual[k])==str(step[k]) for k in ['depth','score','move','nodes'])
  (b/'iteration-control.json').write_text(json.dumps(dict(source=str(tf),expected=step,actual=actual,identical=same),indent=2));assert same,'iteration control differs';break
for i in range(12):
 subprocess.run(['python3',str(b/'win_plan.py')],check=True,stdout=subprocess.DEVNULL)
 s=json.loads((d/'state.json').read_text())
 if s['done'] or not s['next'] or time.monotonic()-start>20 or (b/'PAUSE_QUERIES').exists():break
 j=s['next'][0];p=subprocess.run([str(b/'walk'),j['mode'],str(d),j['id'],j['net'],str(j['budget']),str(j['depth']),j['move'],j['path']],capture_output=True,text=True,timeout=85)
 assert p.returncode in (0,8),(p.returncode,p.stderr)
 r=next(csv.DictReader(open(j['path']),delimiter='\t'));msg=str((j['id'],j['mode'],j['move'],j['budget'],r['depth'],r['score'],r['nodes'],r['ms']));print(msg,flush=True)
 with (d/'log.txt').open('a') as f:f.write(msg+'\n')
subprocess.run(['python3',str(b/'win_plan.py')],check=True)
