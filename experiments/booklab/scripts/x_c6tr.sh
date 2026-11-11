L=~/regicide-jobs/booklab; J=/mnt/c/regicide-match/jobs/booklab
FEN="B:W19,21,22,23,27,28,30,31,32:B2,4,5,6,7,8,12,13,14"
for b in "ACF #61" "ACF #97"; do
  line=$(grep -P "^$b\t" $L/wp14_reach.out); ballot=$(grep -P "^$b\t" $L/wp14_reach_q.tsv | cut -f2); path=$(echo "$line" | cut -f4)
  echo "== $b: $ballot | $path"
  printf "0\t\tB\t0\t%s %s\n" "$ballot" "$path" | $L/bin/blk-pinwalk-1c6e $L/book_r11d.bin ~/regicide-data/egdb | awk -F'\t' '{printf "  ply %d %s eligible %s book %s %s\n", $2+1, $4, $5, $6, ($6==$4 ? "" : "<- not the book move")}'
done
python3 - "$FEN" <<'PY'
import sys, glob, os, collections, subprocess
sys.path.insert(0, '/mnt/c/regicide-match/jobs/booklab/src/tools/booklab')
from theory import parse_pdn, ballot_of, family
c = collections.Counter(); inp = []; meta = []
for d in glob.glob(os.path.expanduser('~/regicide-match/results/bl-*')):
    p = d + '/games.pdn'
    if not os.path.exists(p) or '.' in os.path.basename(d): continue
    for t, m in parse_pdn(open(p, errors='replace').read()):
        if not t['Black'].lower().startswith('regicide') or ballot_of(t['Opening'])[0] != 'ACF #6 Inferno': continue
        inp.append(f"{len(meta)}\t\tB\t0\t{' '.join(x for x, _ in m[:20])}\n"); meta.append(family(t['White']))
out = subprocess.run([os.path.expanduser('~/regicide-jobs/booklab/bin/blk-pinwalk-1c6e'), os.path.expanduser('~/regicide-jobs/booklab/book_r11d.bin')], input=''.join(inp), capture_output=True, text=True).stdout
hit = {int(l.split('\t')[0]) for l in out.splitlines() if len(l.split('\t')) > 2 and l.split('\t')[2] == sys.argv[1]}
tot = collections.Counter(meta); rea = collections.Counter(meta[i] for i in hit)
print('recorded #6 Black games reaching the FEN, by opponent:', {k: f"{rea[k]} of {tot[k]}" for k in tot})
PY
