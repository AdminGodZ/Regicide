# Did the avoid list trigger in the book/avoid test games? Avoid entries = the BLAME lines of avoid-v1's report
# (FEN + move). Each Test-A/B game is replayed with the Linux CLI (first 60 plies); at every position that is an avoid
# entry with Regicide to move, record whether Regicide played the avoided move ("hit") or another move ("dodged").
import re, glob, subprocess, os, sys, collections
E = os.path.expanduser('~/regicide-cc9/build/regicide')
rep = subprocess.run(['git', '-C', os.path.expanduser('~/regicide-nnue'), 'show',
                      'origin/t3/avoid-v1:experiments/avoid-v1/report_v1.txt'], capture_output=True, text=True).stdout
avoid = collections.defaultdict(set)
for m in re.finditer(r'BLAME ply \d+ \d+\.\s+(\S+)\s+FEN (\S+)', rep):
    avoid[m.group(2)].add(m.group(1))
print('avoid entries (distinct fen+move):', sum(len(v) for v in avoid.values()), 'positions:', len(avoid))
tok = re.compile(r'^\d+(?:[-x]\d+)+$')
tags = sys.argv[1:] or ['off-769', 'avoid-769', 'bookavoid-769']
for tag in tags:
    for opp in ('kingsrow', 'cake', 'guinn'):
        files = sorted(glob.glob(os.path.expanduser(f'~/regicide-match/results/ba-{tag}-{opp}/games/*.pdn')))
        if not files: continue
        cmds, meta = [], []
        for f in files:
            txt = open(f).read()
            black_is_reg = re.search(r'\[Black "Regicide', txt) is not None
            body = re.sub(r'\{[^}]*\}', ' ', re.sub(r'\[[^\]]*\]', ' ', txt))
            mv = [t for t in body.split() if tok.match(t)]
            for ply in range(0, min(len(mv), 60)):
                reg_to_move = (ply % 2 == 0) == black_is_reg
                if not reg_to_move: continue
                cmds.append('position startpos moves ' + ' '.join(mv[:ply]) + '\nd\n')
                meta.append((os.path.basename(f), ply, mv[ply]))
        out = subprocess.run([E], input=''.join(cmds) + 'quit\n', capture_output=True, text=True).stdout
        fens = re.findall(r'^([BW]:[^\s]+)$', out, re.M)
        assert len(fens) == len(meta), (len(fens), len(meta))
        hit, dodged, games_hit, games_reached = 0, 0, set(), set()
        for (g, ply, played), fen in zip(meta, fens):
            if fen in avoid:
                games_reached.add(g)
                if played in avoid[fen] or played.replace('x', '-') in avoid[fen]: hit += 1; games_hit.add(g)
                else: dodged += 1
        print(f'{tag:14s} {opp:9s} games {len(files)}: avoid positions reached {hit + dodged} times in {len(games_reached)} games; '
              f'avoided move played {hit} (in {len(games_hit)} games), other move {dodged}')
