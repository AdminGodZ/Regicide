"""Pool match result dirs (results.csv of tools/match) into W/D/L, pentanomial pairs and Elo with a 95% CI (as
jobs/tm/pool.py: per-pair mean score, normal approximation, delta method).   pool_dirs.py DIR..."""
import collections, csv, math, os, sys
W = D = L = 0; penta = [0] * 5; pairs = []
for d in sys.argv[1:]:
    byp = collections.defaultdict(list)
    for r in csv.DictReader(open(os.path.join(d, "results.csv"))):
        s = float(r["score_a"]); byp[r["pair"]].append(s)
        W += s == 1; D += s == 0.5; L += s == 0
    for v in byp.values():
        if len(v) == 2:
            penta[int(round(sum(v) * 2))] += 1; pairs.append(sum(v) / 2)
n = len(pairs); m = sum(pairs) / n; se = math.sqrt(sum((x - m) ** 2 for x in pairs) / (n - 1) / n)
elo = lambda s: -400 * math.log10(1 / s - 1)
slope = 400 / math.log(10) / (m * (1 - m))
z = (m - 0.5) / se if se > 0 else 0
los = 0.5 * (1 + math.erf(z / math.sqrt(2)))
print("dirs %d  W/D/L %d/%d/%d  pairs %d  pentanomial [LL LD DD/WL WD WW] = %s" % (len(sys.argv) - 1, W, D, L, n, penta))
print("Elo %+.1f [%+.1f, %+.1f] 95%% CI (pentanomial, pairs), score %.4f +- %.4f, LOS %.1f%%" % (
    elo(m), elo(m) - 1.96 * se * slope, elo(m) + 1.96 * se * slope, m, 1.96 * se, 100 * los))
