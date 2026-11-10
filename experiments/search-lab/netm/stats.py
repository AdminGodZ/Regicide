# stats.py NAME_A DIR...: pooled per-side mean time and depth per move from the PDN comments, terminations and any
# forfeits. Depth: over moves the engine searched (depth >= 2; forced replies report depth 1); the first 20 such
# moves of each side per game (middlegame) and all of them (database endgames reach very high depths).
import re, sys, os, collections
name_a = sys.argv[1]
t = {'A': [0.0, 0], 'B': [0.0, 0]}; d = {'A': [0, 0], 'B': [0, 0]}; e = {'A': [0, 0], 'B': [0, 0]}
term = collections.Counter(); odd = []
for dr in sys.argv[2:]:
    txt = open(os.path.join(dr, 'games.pdn')).read()
    for g in re.split(r'\n(?=\[Event )', txt):
        if not g.strip(): continue
        tags = dict(re.findall(r'\[(\w+) "([^"]*)"\]', g))
        tm = tags.get('Termination', '')
        term[tm] += 1
        if re.search(r'time|illegal|forfeit|crash|kill', tm, re.I): odd.append((dr, tags.get('Round'), tm))
        side = 'B' if tags.get('FEN', 'B').startswith('B') else 'W'
        body = g[g.rfind(']') + 1:]
        toks = re.findall(r'\{[^}]*\}|\d+\.(?!\d)|[0-9]+[-x][0-9x-]+', body)
        mover = None; cnt = {'A': 0, 'B': 0}
        for tok in toks:
            if tok.endswith('.') : continue
            if tok.startswith('{'):
                m = re.match(r'\{([\d.]+)s depth (\d+)', tok.replace('\n', ' '))
                if m and mover:
                    who = 'A' if tags[mover] == name_a else 'B'
                    t[who][0] += float(m.group(1)); t[who][1] += 1
                    if int(m.group(2)) >= 2:
                        d[who][0] += int(m.group(2)); d[who][1] += 1
                        cnt[who] += 1
                        if cnt[who] <= 20: e[who][0] += int(m.group(2)); e[who][1] += 1
                continue
            mover = 'Black' if side == 'B' else 'White'
            side = 'W' if side == 'B' else 'B'
for k in 'AB':
    print(f'{k}: moves {t[k][1]} mean {t[k][0]/max(1,t[k][1]):.3f} s/move, mean depth first 20 searched moves {e[k][0]/max(1,e[k][1]):.2f}, all searched moves {d[k][0]/max(1,d[k][1]):.2f}')
print('terminations:', dict(term))
print('forfeits/illegal:', odd if odd else 'none')
