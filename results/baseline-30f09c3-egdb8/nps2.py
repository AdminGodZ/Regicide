import re, glob, statistics
pat = re.compile(r'\{(\d+\.\d+)s depth (\d+)/\d+ score (-?\d+) nodes (\d+) (\d+) kN/s')
for opp in ['kingsrow', 'cake', 'guinn']:
    for name in ['eb1037c-n4c', '30f09c3-egdb8']:
        row = []
        for lo in range(0, 312, 52):
            k = []
            for g in range(lo + 1, lo + 53):
                try: txt = open(f'/home/admingod/regicide-match/results/baseline-{name}-{opp}/games/{g:05d}.pdn').read().replace('\n', ' ')
                except FileNotFoundError: continue
                k += [int(m.group(5)) for m in pat.finditer(txt) if float(m.group(1)) >= 0.3]
            row.append(f"{statistics.median(k):5.0f}")
        print(f"{opp:9s} {name:14s} median kN/s by game block of 52: {' '.join(row)}")
