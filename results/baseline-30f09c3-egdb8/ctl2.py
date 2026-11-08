import re, glob, statistics
pat = re.compile(r'\{(\d+\.\d+)s depth (\d+)/\d+ score (-?\d+) nodes (\d+) (\d+) kN/s')
for name in ['eb1037c-n4c-kingsrow', '30f09c3-egdb8-kingsrow', '30f09c3-kingsrow']:
    k, d, n, inst = [], [], 0, 0
    for f in glob.glob(f'/home/admingod/regicide-match/results/baseline-{name}/games/*.pdn'):
        for m in pat.finditer(open(f).read().replace('\n', ' ')):
            n += 1; s = float(m.group(1))
            if s < 0.05: inst += 1
            if s >= 0.3: k.append(int(m.group(5))); d.append(int(m.group(2)))
    print(f"{name:24s} moves {n:6d} instant {100*inst/n:5.1f}%  median kN/s {statistics.median(k):5.0f}  depth {statistics.median(d):.0f}")
