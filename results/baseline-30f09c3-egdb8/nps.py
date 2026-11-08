import re, glob, statistics, sys
# Regicide comments: "{0.71s depth 17/31 score -39 nodes 1629203 2253 kN/s pv ..." (opponent comments use "value=")
pat = re.compile(r'\{(\d+\.\d+)s depth (\d+)/\d+ score (-?\d+) nodes (\d+) (\d+) kN/s')
for opp in ['kingsrow', 'cake', 'guinn']:
    for name in ['eb1037c-n4c', '30f09c3-egdb8']:
        knps, depth, t, n, inst = [], [], [], 0, 0
        for f in glob.glob(f'/home/admingod/regicide-match/results/baseline-{name}-{opp}/games/*.pdn'):
            txt = open(f).read().replace('\n', ' ')
            for m in pat.finditer(txt):
                s = float(m.group(1)); n += 1
                if s < 0.05: inst += 1; continue
                t.append(s)
                if s >= 0.3: knps.append(int(m.group(5))); depth.append(int(m.group(2)))
        print(f"{opp:9s} {name:14s} moves {n:6d}  instant(<0.05s) {100*inst/n:5.1f}%  mean t(searched) {statistics.mean(t):.3f}s  "
              f"median kN/s (>=0.3s) {statistics.median(knps):6.0f}  median depth {statistics.median(depth):.0f}")
