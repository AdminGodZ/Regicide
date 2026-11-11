"""Pool match result dirs (results.csv of tools/match) into W/D/L, pentanomial pairs and Elo with a 95% CI (as
jobs/tm/pool.py: per-pair mean score, normal approximation, delta method).   pool_dirs.py DIR..."""
import collections, csv, math, os, sys
W = D = L = 0; penta = [0] * 5; pairs = []
for d in sys.argv[1:]:
    # Duplicated or misplaced game rows are refused (review r17/r18). The arm comes from the runner's own numbering
    # (tools/match/main.cpp: task t -> game t+1, pair t/2+1, leg t%2, engine A is Black in leg 0), never from engine
    # names: both engines of a net match report the same name. So a pair p holds at most games 2p-1 (A Black) and 2p
    # (A White); a game id twice is refused, and score_a must agree with the result and A's colour.
    byp, seen = collections.defaultdict(list), set()
    for r in csv.DictReader(open(os.path.join(d, "results.csv"))):
        if r["game"] in seen:
            sys.exit("%s: game %s appears twice in results.csv" % (d, r["game"]))
        seen.add(r["game"])
        g, p = int(r["game"]), int(r["pair"])
        if (g - 1) // 2 + 1 != p:
            sys.exit("%s: game %d is listed in pair %d (the runner puts it in pair %d)" % (d, g, p, (g - 1) // 2 + 1))
        a_black = g % 2 == 1
        exp = {"1-0": 1.0 if a_black else 0.0, "0-1": 0.0 if a_black else 1.0, "1/2-1/2": 0.5}.get(r["result"])
        if exp is None or float(r["score_a"]) != exp:
            sys.exit("%s: game %d: score_a %s does not match result %s with A as %s" % (
                d, g, r["score_a"], r["result"], "Black" if a_black else "White"))
        byp[p].append(r)
    for p, rs in byp.items():
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
