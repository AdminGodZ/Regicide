"""Pool match result dirs (results.csv of tools/match) into W/D/L, pentanomial pairs and Elo with a 95% CI (as
jobs/tm/pool.py: per-pair mean score, normal approximation, delta method).   pool_dirs.py DIR..."""
import collections, csv, math, os, sys
W = D = L = 0; penta = [0] * 5; pairs = []
for d in sys.argv[1:]:
    # duplicated game rows are refused (review r17): a game number twice, a pair with more than two games, or a pair
    # whose two games have the same engine as Black would otherwise be pooled as if they were sound pairs
    byp, seen = collections.defaultdict(list), set()
    for r in csv.DictReader(open(os.path.join(d, "results.csv"))):
        if r["game"] in seen:
            sys.exit("%s: game %s appears twice in results.csv" % (d, r["game"]))
        seen.add(r["game"])
        byp[r["pair"]].append(r)
    for p, rs in byp.items():
        if len(rs) > 2:
            sys.exit("%s: pair %s has %d games" % (d, p, len(rs)))
        if len(rs) == 2 and rs[0]["black"] == rs[1]["black"]:
            sys.exit("%s: pair %s plays %s as Black in both games" % (d, p, rs[0]["black"]))
        v = [float(r["score_a"]) for r in rs]
        for s in v:
            W += s == 1; D += s == 0.5; L += s == 0
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
