from pathlib import Path
import json,re
b=Path(__file__).resolve().parent
gs=json.loads((b/'records.json').read_text())
new=[g for g in json.loads((b/'validation-games.json').read_text())if g['run'].startswith('mo3-')]
assert len(gs)==23720 and len(new)==936
assert len((b/'records.tsv').read_text().splitlines())==len(gs),'append once only'
with open(b/'records.tsv','a')as f:
 for i,g in enumerate(new,len(gs)):
  searched=','.join(str(j+1)for j,c in enumerate(g['comments'])if(m:=re.search(r'\bnodes\s+(\d+)',c))and int(m[1])>0)
  f.write('\t'.join([str(i),g['run'],g['own'],g['tags'].get('Result','*'),g['tags'].get('FEN',''),' '.join(g['moves']),searched])+'\n')
