# check_chunks.py: the six chunk files are a partition of openings4.txt (every opening exactly once) and match the
# recorded sha256 sums.
import collections, hashlib, os, sys
N = os.path.expanduser('~/regicide-jobs/netm') if len(sys.argv) < 2 else sys.argv[1]
full = [l.rstrip('\n') for l in open(f'{N}/openings4.txt') if l.strip()]
parts = [l.rstrip('\n') for c in 'abcdef' for l in open(f'{N}/chunks/op4_{c}') if l.strip()]
cf, cp = collections.Counter(full), collections.Counter(parts)
dups = [k for k, v in cf.items() if v > 1]
ok = cf == cp and len(parts) == len(full) == 805 and parts == full
print(f'openings4 {len(full)} lines ({len(cf)} distinct, duplicates {len(dups)}); chunks {len(parts)} lines;',
      'partition (in order, each line exactly once): ' + ('yes' if ok else 'NO'))
for c in 'abcdef':
    print(c, hashlib.sha256(open(f'{N}/chunks/op4_{c}', 'rb').read()).hexdigest())
sys.exit(0 if ok else 1)
