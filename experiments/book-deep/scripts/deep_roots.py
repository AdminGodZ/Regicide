# Roots for the deep targeted book: the position after the 3-move ballot (White to move) of every ballot Regicide lost,
# as Black or as White, in today's n4c-engine CheckerBoard runs: baseline-eb1037c-n4c-* (7-piece, incl. 2 s),
# baseline-30f09c3-egdb8-* and baseline-30f09c3-kingsrow (same engine, 8/7-piece), ba-* (65f7143 / 769ef0b / 9841716 /
# b88c02e book/avoid/tm runs incl. 2 s, Cake 2 s). Writes roots.txt (one FEN per line) and roots_ballots.txt.
import re, glob, os, csv, subprocess, collections
R = os.path.expanduser('~/regicide-match/results')
E = os.path.expanduser('~/regicide-cc9/build/regicide')
OUT = os.path.expanduser('~/regicide-jobs/deep')
dirs = sorted(glob.glob(f'{R}/baseline-eb1037c-n4c-*') + glob.glob(f'{R}/baseline-30f09c3-egdb8-*') +
              [f'{R}/baseline-30f09c3-kingsrow'] + glob.glob(f'{R}/ba-*'))
lost = collections.defaultdict(list)      # ballot -> [(run, colour)]
moves_of = {}
for d in dirs:
    if not os.path.exists(f'{d}/results.csv'): continue
    for r in csv.DictReader(open(f'{d}/results.csv')):
        if r['score_a'] != '0': continue
        txt = open(f'{d}/games/{int(r["game"]):05d}.pdn').read()
        op = re.search(r'\[Opening "([^"]+)"\]', txt).group(1)
        name, mv = op.split(':', 1)
        col = 'B' if re.search(r'\[Black "Regicide', txt) else 'W'
        lost[name.strip()].append((os.path.basename(d), col))
        moves_of[name.strip()] = mv.split()
names = sorted(lost, key=lambda n: (-len(lost[n]), n))
out = subprocess.run([E], input=''.join(f'position startpos moves {" ".join(moves_of[n])}\nd\n' for n in names) + 'quit\n',
                     capture_output=True, text=True).stdout
fens = re.findall(r'^([BW]:[^\s]+)$', out, re.M)
assert len(fens) == len(names)
seen = []
with open(f'{OUT}/roots.txt', 'w') as fr, open(f'{OUT}/roots_ballots.txt', 'w') as fb:
    fb.write(f'{"ballot":30s} {"moves":18s} {"losses":>6s} {"as B":>5s} {"as W":>5s}  runs (opponent)\n')
    for n, f in zip(names, fens):
        if f not in seen: seen.append(f); fr.write(f + '\n')
        c = collections.Counter(col for _, col in lost[n])
        opp = collections.Counter(re.sub(r'^(baseline-[^-]+(-n4c)?(-egdb8)?|ba-[^-]+(-[^-]+)?)-', '', run) for run, _ in lost[n])
        fb.write(f'{n:30s} {" ".join(moves_of[n]):18s} {len(lost[n]):6d} {c["B"]:5d} {c["W"]:5d}  {dict(opp)}  {f}\n')
print(f'{len(dirs)} run dirs, {sum(len(v) for v in lost.values())} lost games, {len(names)} ballots, {len(seen)} distinct roots')
print('lost as Black only:', sum(1 for n in names if all(c == "B" for _, c in lost[n])),
      ' as White only:', sum(1 for n in names if all(c == "W" for _, c in lost[n])), ' both:',
      sum(1 for n in names if len({c for _, c in lost[n]}) == 2))
