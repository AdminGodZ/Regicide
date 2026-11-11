import csv, collections, os
d = os.path.expanduser('~/regicide-jobs/booklab/wp12/c2')
rows = [l.rstrip('\n').split('\t') for l in open(d + '/candidates.tsv') if not l.startswith('#')]
h = rows[0]; cands = [dict(zip(h, r)) for r in rows[1:]]
print(len(cands), 'candidates; arms:', dict(collections.Counter(c['arm'] for c in cands)))
print('rank 0:', {k: cands[0][k] for k in ('arm', 'ballot', 'colour', 'ply', 'pinned', 'pinned_score', 'alternative', 'alt_score')})
cons = [l.rstrip('\n').split('\t') for l in open(d + '/considered.tsv') if not l.startswith('#')]
ch = cons[0]; cons = [dict(zip(ch, r)) for r in cons[1:]]
kept = sum(1 for c in cons if c['kept'] == '1')
why = collections.Counter((c['why'].split(':')[0][:60] if c['kept'] == '0' else 'kept') for c in cons)
print('considered', len(cons), 'kept', kept); [print('  ', n, w) for w, n in why.most_common(8)]
diff = [int(c['alt_score']) - int(c['pinned_score']) for c in cands[1:] if c['alt_score'] not in ('None', '') and c['pinned_score'] not in ('None', '')]
print('alternative - pinned over candidates: min', min(diff), 'median', sorted(diff)[len(diff)//2], 'max', max(diff), '; >= 0:', sum(d >= 0 for d in diff))
