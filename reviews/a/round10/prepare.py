import pathlib,gzip,re,json,csv,hashlib
b=pathlib.Path(__file__).resolve().parent
def parse(raw):
 tags=dict(re.findall(r'^\[(\w+) "([^"]*)"\]\s*$',raw,re.M));body=re.sub(r'^\[[^\n]*\]\s*','',raw,flags=re.M);ms=[];cs=[]
 for tok in re.findall(r'\{[^}]*\}|\S+',body):
  if tok.startswith('{'):
   if ms:cs[-1]+=tok[1:-1]
  elif tok not in ['1-0','0-1'] and re.fullmatch(r'\d+(?:[-x:]\d+)+',tok):ms.append(tok);cs.append('')
 return tags,ms,cs
records=[];sources=[]
for p in sorted((b/'booklab-src/experiments/booklab').rglob('*.pdn.gz')):
 raw=gzip.open(p,'rt',errors='replace').read();n=0
 for i,g in enumerate(re.split(r'(?=\[Event )',raw)):
  if not g.strip():continue
  tags,ms,cs=parse(g)
  if not ms or 'Black'not in tags:continue
  own='B'if tags['Black'].lower().startswith('regicide')else'W'if tags.get('White','').lower().startswith('regicide')else'-'
  if own=='-':continue
  run=p.parent.name if p.name=='games.pdn.gz'else p.name[:-7]
  records.append(dict(id=str(len(records)),run=run,path=str(p.relative_to(b/'booklab-src')),index=i,tags=tags,moves=ms,comments=cs,own=own))
  n+=1
 sources.append(dict(path=str(p.relative_to(b/'booklab-src')),games=n,sha256=hashlib.sha256(raw.encode()).hexdigest()))
(b/'records.json').write_text(json.dumps(records))
with open(b/'records.tsv','w')as f:
 for g in records:f.write('\t'.join([g['id'],g['run'],g['own'],g['tags'].get('Result','*'),g['tags'].get('FEN',''),' '.join(g['moves'])])+'\n')
with open(b/'tables/record-sources.tsv','w')as f:
 w=csv.DictWriter(f,fieldnames=sources[0],delimiter='\t',lineterminator='\n');w.writeheader();w.writerows(sources)
print('PDN files',len(sources),'games',len(records),'plies',sum(len(g['moves'])for g in records))
# Dip requests preserve complete game history; independent replay is done by C++.
ds=list(csv.DictReader(open(b/'booklab-src/experiments/booklab/wp14/dips.tsv'),delimiter='\t'))
with open(b/'dips-requests.tsv','w')as f:
 for i,d in enumerate(ds,1):
  for n in [1000000,3000000,10000000]:
   f.write(f"{i}-{n}\t{d['moves_from_start']}\t{d['fen']}\t{n}\n")
print('dips',len(ds),'unique FENs',len(set(d['fen']for d in ds)))
