"""Per-ballot comparison of CheckerBoard runs (results.csv of tools/match, A = Regicide).

  python ballots.py "n8bn=DIR1,DIR2" "n6c=DIR3,DIR4"

For every group: W/D/L and the games A won or lost (ballot, A's colour, run). Then the ballots whose summed score
differs between the groups (per run, so groups with different numbers of runs are compared per run).
"""
import collections
import csv
import os
import sys


def load(dirs):
    games = []
    for d in dirs:
        for r in csv.DictReader(open(os.path.join(d, "results.csv"))):
            colour = "B" if r["black"].startswith("Regicide") else "W"
            games.append((os.path.basename(d.rstrip("/")), r["opening"], colour, float(r["score_a"])))
    return games


def main():
    groups = []
    for arg in sys.argv[1:]:
        name, dirs = arg.split("=", 1)
        groups.append((name, dirs.split(","), load(dirs.split(","))))
    per = {}
    for name, dirs, games in groups:
        w = sum(g[3] == 1 for g in games)
        d = sum(g[3] == 0.5 for g in games)
        l = sum(g[3] == 0 for g in games)
        print("## %s (%d runs): +%d =%d -%d" % (name, len(dirs), w, d, l))
        for run, op, col, s in sorted(games, key=lambda g: (g[1], g[2], g[0])):
            if s != 0.5:
                print("- %s %s as %s: %s" % (op, run, "black" if col == "B" else "white", "win" if s == 1 else "loss"))
        acc = collections.defaultdict(float)
        for run, op, col, s in games:
            acc[(op, col)] += (s - 0.5) / len(dirs)
        per[name] = acc
        print()
    if len(groups) == 2:
        a, b = groups[0][0], groups[1][0]
        keys = sorted(set(per[a]) | set(per[b]))
        diff = [(k, per[a].get(k, 0), per[b].get(k, 0)) for k in keys if abs(per[a].get(k, 0) - per[b].get(k, 0)) > 1e-9]
        print("## ballots where %s and %s differ (score - 0.5 per game, averaged over runs)" % (a, b))
        print("| ballot | Regicide plays | %s | %s |" % (a, b))
        print("|---|---|---|---|")
        for (op, col), x, y in diff:
            print("| %s | %s | %+.2f | %+.2f |" % (op, "black" if col == "B" else "white", x, y))


if __name__ == "__main__":
    main()
