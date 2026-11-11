from pathlib import Path
import csv,subprocess
b=Path(__file__).resolve().parent;r=b.parent.parent
new=b/'claim-src/build/tools/match';old=r/'reports/round6/match-build/match';host=b/'claim-src/build/tools/cbhost';dll=r/'reports/round5/mock_engine.so';fake=b/'claim-src/build/tools/runner_test';d=b/'runner-fixtures';d.mkdir(exist_ok=True)
def run(label,exe,decktext,options,claim=0,a=None,bb=None):
 deck=d/(label+'.deck');deck.write_text(decktext+'\n');out=d/label
 args=[str(exe),'-a',a or f'exec:{host} {dll}','-b',bb or f'exec:{host} {dll}','--openings',str(deck),'-c','1','--pairs-together','--movetime','.01','--pdn-comments','-o',str(out)]
 if a is None:args+=['--option-a',f'set claim {claim}','--option-b',f'set claim {claim}']
 p=subprocess.run(['nice','-n','19']+args+options,text=True,capture_output=True,timeout=30)
 if p.returncode:raise RuntimeError((label,p.returncode,p.stderr))
 return list(csv.DictReader(open(out/'results.csv')))
for claim in range(4):
 a=run(f'before-default-{claim}',old,'B:WK32:BK1',[],claim)
 c=run(f'after-default-{claim}',new,'B:WK32:BK1',[],claim)
 fields=['game','pair','black','white','result','score_a','termination','engine_plies','first_claim']
 assert [{k:x[k] for k in fields}for x in a]==[{k:x[k]for k in fields}for x in c]
print('default: all original result fields and claim metadata identical in 8 games')
for label,deck,args,claim,a,bb in [
 ('terminal-win-limit1-repeat','B:W10:B6',['--cb-claims','--cb-move-limit','1'],1,None,None),
 ('terminal-win-no-limit','B:W10:B6',['--cb-claims'],1,None,None),
 ('terminal-draw-no-limit','B:W10:B6',['--cb-claims'],0,None,None),
 ('fourth-draw-limit3','B:WK32:BK1',['--cb-claims','--cb-move-limit','3'],0,f'exec:{fake} --fake-claims draw 4',f'exec:{fake} --fake-claims unknown 1')]:
 rows=run(label,new,deck,args,claim,a,bb)
 print(label,[(x['result'],x['score_a'],x['termination'],x['engine_plies'],x['first_claim'])for x in rows])
