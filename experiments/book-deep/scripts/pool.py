# Pool CheckerBoard runs (results.csv of tools/match: game, pair, ..., score_a) into W/D/L, pentanomial pair counts,
# and an Elo estimate with a 95% CI (pentanomial: per-pair mean score and its standard error; normal approximation,
# Elo = -400 log10(1/s - 1), CI by the delta method). Also the difference between two pooled groups with a 95% CI
# (independent samples: se = sqrt(se1^2 + se2^2), in Elo via the slope at each group's score).
#   pool.py "NAME=run1,run2" "NAME=run3,run4,run5"      (run = results dir name under ~/regicide-match/results)
import csv, math, os, sys, collections
R = os.path.expanduser('~/regicide-match/results')
def load(runs):
    W = D = L = 0; penta = [0] * 5; pairs = []
    for run in runs:
        rows = list(csv.DictReader(open(f'{R}/{run}/results.csv')))
        byp = collections.defaultdict(list)
        for r in rows:
            s = float(r['score_a']); byp[(run, r['pair'])].append(s)
            W += s == 1; D += s == 0.5; L += s == 0
        for k, v in byp.items():
            if len(v) == 2:
                t = sum(v); penta[int(round(t * 2))] += 1; pairs.append(t / 2)
    return W, D, L, penta, pairs
def elo(s): return -400 * math.log10(1 / s - 1)
def stats(pairs):
    n = len(pairs); m = sum(pairs) / n
    var = sum((x - m) ** 2 for x in pairs) / (n - 1); se = math.sqrt(var / n)
    slope = 400 / math.log(10) / (m * (1 - m))       # dElo/ds
    return m, se, elo(m), 1.96 * se * slope, slope
groups = []
for arg in sys.argv[1:]:
    name, runs = arg.split('=', 1); runs = runs.split(',')
    W, D, L, penta, pairs = load(runs)
    m, se, e, ci, slope = stats(pairs)
    groups.append((name, m, se, e, slope))
    print(f'{name}: runs {", ".join(runs)}')
    print(f'   games {W + D + L}: W/D/L {W}/{D}/{L}; pairs {len(pairs)}, pentanomial [LL LD DD/WL WD WW] = {penta}')
    print(f'   score {m:.4f} +- {1.96 * se:.4f}; Elo (pentanomial) {e:+.1f} [{e - ci:+.1f}, {e + ci:+.1f}] 95% CI')
if len(groups) == 2:
    (a, ma, sa, ea, ka), (b, mb, sb, eb, kb) = groups
    d = ea - eb; se_d = math.sqrt((sa * ka) ** 2 + (sb * kb) ** 2)
    z = d / se_d; los = 0.5 * (1 + math.erf(z / math.sqrt(2)))
    print(f'{a} - {b}: {d:+.1f} Elo [{d - 1.96 * se_d:+.1f}, {d + 1.96 * se_d:+.1f}] 95% CI, LOS {100 * los:.1f}%')
