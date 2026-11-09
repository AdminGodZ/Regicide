# Prints a RESULTS.md table row for each named test log: python3 -I row.py name...
import re, sys, os, math
os.chdir(os.path.expanduser('~/regicide-jobs/searchlab'))
for name in sys.argv[1:]:
    t = open(f'logs/{name}.log').read()
    m = re.findall(r'final games (\d+)\s+A: \+(\d+) =(\d+) -(\d+)\s+score ([\d.]+)\s+draws ([\d.]+)%\s+elo ([+-][\d.]+) \[([+-][\d.]+), ([+-][\d.]+)\]\s+penta \[(\d+) (\d+) (\d+) (\d+) (\d+)\]', t)
    if not m:
        m = re.findall(r'games (\d+)\s+A: \+(\d+) =(\d+) -(\d+)\s+score ([\d.]+)\s+draws ([\d.]+)%\s+elo ([+-][\d.]+) \[([+-][\d.]+), ([+-][\d.]+)\]\s+penta \[(\d+) (\d+) (\d+) (\d+) (\d+)\]', t)
        tag = ' (partial)'
    else:
        tag = ''
    g = m[-1]
    n, w, d, l = map(int, g[:4]); elo, lo, hi = map(float, g[6:9]); p = list(map(int, g[9:]))
    pairs = sum(p)
    half = (hi - lo) / 2
    # error bar scaled to 800 pairs; t = elo / se
    se = half / 1.96
    tA = re.search(r'time A: moves \d+\s+mean ms ([\d.]+)\s+mean depth ([\d.]+)', t)
    tB = re.search(r'time B: moves \d+\s+mean ms ([\d.]+)\s+mean depth ([\d.]+)', t)
    tm = f'{tA.group(1)} / {tB.group(1)} ms, d {tA.group(2)} / {tB.group(2)}' if tA and tB else ''
    print(f'| {name}{tag} | {w} / {d} / {l} | {float(g[5]):.1f}% | {elo:+.1f} [{lo:+.1f}, {hi:+.1f}] | '
          f'{" ".join(map(str,p))} | {pairs} | ±{half*math.sqrt(pairs/800):.1f} | {elo/se if se else 0:+.2f} | {tm} |')
