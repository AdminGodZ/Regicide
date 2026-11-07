# Usage: nps.py games.pdn  -> per-engine match nps (sum nodes / sum time, moves with >= 10k nodes)
import re, sys
from collections import defaultdict
txt = open(sys.argv[1]).read()
nodes = defaultdict(float); secs = defaultdict(float); depth = defaultdict(list)
for g in re.split(r'\n(?=\[Event )', txt):
    tags = dict(re.findall(r'\[(\w+) "([^"]*)"\]', g))
    if 'Black' not in tags: continue
    body = re.sub(r'\[[^\]]*\]', '', g)
    ply = -1
    for tok in re.finditer(r'\{([^}]*)\}|(\d+\.(?:\.\.)?)|(\d+(?:[-x]\d+)+)', body):
        if tok.group(3): ply += 1
        elif tok.group(1):
            c = ' '.join(tok.group(1).split())
            m = re.search(r'depth (\d+)/\d+ score \S+ nodes (\d+) (\d+) kN/s', c)
            if not m: continue
            n, k = int(m.group(2)), int(m.group(3))
            if n < 10000 or k == 0: continue
            eng = tags['Black'] if ply % 2 == 0 else tags['White']
            nodes[eng] += n; secs[eng] += n / (k * 1000.0); depth[eng].append(int(m.group(1)))
for e in sorted(nodes):
    print(f"{e}: {nodes[e]/secs[e]/1e6:.3f} Mnps over {len(depth[e])} moves, mean depth {sum(depth[e])/len(depth[e]):.2f}")
