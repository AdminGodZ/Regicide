import os, re, sys
sys.path.insert(0, os.path.expanduser("~/regicide-booklab/tools/booklab"))
from theory import parse_pdn, family, ballot_of, acf_name
R = os.path.expanduser("~/regicide-match/results")
SC = re.compile(r"\bscore\s+(-?\d+)")
def games(run):
    out = {}
    for t, m in parse_pdn(open(f"{R}/{run}/games.pdn").read()):
        _, om, n = ballot_of(t["Opening"])
        col = "B" if family(t["Black"]) == "regicide" else "W"
        out[(om, col)] = (t, m, n)
    return out
for run, opp in (("bl-f4-cake-h1", "cake"), ("bl-f4-cake-h2", "cake")):
    h = run[-2:]
    g4 = games(run)
    fa, fb = games(f"bl-f1-{opp}-a-{h}"), games(f"bl-f1-{opp}-b-{h}")
    for (om, col), (t, m, n) in g4.items():
        res = t["Result"]
        lost = (res == "1-0" and col == "W") or (res == "0-1" and col == "B")
        if not lost:
            continue
        mv = [x for x, _ in m]
        line = f"{acf_name(om)} ({om}) Regicide {col}, {run} round {t['Round']}, {len(mv)} plies"
        for nm, f in (("final a", fa), ("final b", fb)):
            o = f.get((om, col))
            if not o:
                continue
            ov = [x for x, _ in o[1]]
            k = next((i for i in range(min(len(mv), len(ov))) if mv[i] != ov[i]), min(len(mv), len(ov)))
            who = "Regicide" if ((k % 2 == 0) == (col == "B")) else "Cake"
            line += f"; vs {nm} ({o[0]['Result']}): first difference ply {k} ({k - n} after the ballot) by {who}: n8q {mv[k] if k < len(mv) else '-'} / n6c {ov[k] if k < len(ov) else '-'}"
        # Regicide scores after the ballot
        sc = []
        for i in range(n, len(mv)):
            if (i % 2 == 0) == (col == "B") and m[i][1]:
                s = SC.search(m[i][1])
                if s:
                    sc.append((i, int(s.group(1))))
        neg = next(((i, s) for i, s in sc if s <= -100), None)
        line += f"; first own score <= -100 at ply {neg[0]} ({neg[1]})" if neg else "; own score never <= -100"
        print(line)
