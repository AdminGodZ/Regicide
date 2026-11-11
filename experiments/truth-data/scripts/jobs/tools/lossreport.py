"""Aggregate the loss analysis (lw_*.tsv of truth losswalk) into markdown: per game and per opponent/net.
The static eval at the turning move is computed with the net the game was played with (regicide CLI, evalpos)."""
import csv
import glob
import os
import statistics
import subprocess

J = os.path.expanduser("~/regicide-jobs/truth")
E = os.path.expanduser("~/regicide-truth/build/regicide")
N = {"n4c": "~/regicide-nnue4-work/n4c.nnue", "n6c": "~/regicide-nnue6-work/n6c.nnue",
     "n8bn": "~/regicide-truth-work/n8bn.nnue", "n8c": "~/regicide-truth-work/n8c.nnue", "n8q": "~/regicide-truth-work/n8q.nnue"}


def net_of(run):
    if run.startswith(("pt-s2-", "tr-n6c-", "ba-deep-off")):
        return "n6c"
    if run.startswith(("ba-off-769", "ba-tm-off")):
        return "n4c"
    if run.startswith("tr-n8bn"):
        return "n8bn"
    if run.startswith("tr-n8c"):
        return "n8c"
    if run.startswith("bl-f4"):
        return "n8q"
    return "?"


rows = []
for f in sorted(glob.glob(os.path.join(J, "lw_*.tsv"))):
    run = os.path.basename(f)[3:-4]
    for r in csv.DictReader(open(f), delimiter="\t"):
        r["run"] = run
        r["net"] = net_of(run)
        r["opp"] = "Cake" if "cake" in run else "GuiNN" if "guinn" in run else "?"
        rows.append(r)
ok = [r for r in rows if r.get("pieces_before", "").isdigit()]
bad = [r for r in rows if r not in ok]
# static evals at the turning move (Regicide's view): before as is, after negated (opponent to move there)
evals = {}
for net, path in N.items():
    sel = [r for r in ok if r["net"] == net and r.get("turn_fen_before")]
    if not sel:
        continue
    cmds = ["set evalfile " + os.path.expanduser(path)]
    for r in sel:
        cmds += ["evalpos " + r["turn_fen_before"], "evalpos " + r["turn_fen_after"]]
    out = subprocess.run([E], input="\n".join(cmds) + "\nquit\n", capture_output=True, text=True).stdout
    vals = [int(l.split()[1]) for l in out.splitlines() if l.startswith("eval ")]
    for i, r in enumerate(sel):
        evals[id(r)] = (vals[2 * i], -vals[2 * i + 1])


def i(x):
    try:
        return int(x)
    except (TypeError, ValueError):
        return None


def ingame(r):
    """Regicide's in-game score at the turning move, or None if the game has none there. Tested for None explicitly:
    a real score of 0 is a score (review r17: `or -9999` had turned 0 into "missing"). truth losswalk writes 99999
    when the move's comment holds no score."""
    s = i(r["turn_ingame_score"])
    return None if s is None or s == 99999 else s


print("### Lost games, one row each\n")
print("| run | net | opp | ballot | Regicide | proven lost: move (pieces) | turn: pieces | deep score before → after | in-game score | static eval before → after |")
print("|---|---|---|---|---|---|---|---|---|---|")
for r in ok:
    ev = evals.get(id(r), (None, None))
    print("| %s | %s | %s | %s | %s | %s (%s) | %s | %s → %s | %s | %s → %s |" % (
        r["run"], r["net"], r["opp"], r["opening"].rsplit(": ", 1)[-1], r["regicide"], r["move_no"], r["pieces_before"],
        r["turn_pieces"], r["turn_deep_before"], r["turn_deep_after"], r["turn_ingame_score"], ev[0], ev[1]))
for r in bad:
    print("| %s | %s | %s | %s | %s | %s | | | | |" % (r["run"], r["net"], r["opp"], r["opening"].rsplit(": ", 1)[-1],
                                                     r["regicide"], r.get("ply", "")))


def bins(vals, edges=((20, 24), (17, 19), (13, 16), (9, 12), (2, 8))):
    return " / ".join("%d" % sum(lo <= v <= hi for v in vals) for lo, hi in edges)


print("\n### Summary by opponent and net\n")
print("Turning moves whose deep score drops by 100 or more: 'horizon' if Regicide's in-game score for the move stayed above "
      "−50, 'eval' if the net's static eval of the position after the move is ≥ 0.\n")
print("| opponent | net | lost games | proven lost: median pieces | already ≤ −50 at Regicide's first decision | turn: games at 20–24 / 17–19 / 13–16 / 9–12 / ≤ 8 pieces (others) | drops ≥ 100 at the turn | of which horizon / eval |")
print("|---|---|---|---|---|---|---|---|")
for opp in ("Cake", "GuiNN"):
    for net in ("n4c", "n6c", "n8bn", "n8c", "n8q"):
        sel = [r for r in ok if r["opp"] == opp and r["net"] == net]
        if not sel:
            continue
        proven = [int(r["pieces_before"]) for r in sel]
        early = [r for r in sel if (i(r["turn_deep_before"]) or 0) < -50]
        turn = [int(r["turn_pieces"]) for r in sel if i(r["turn_pieces"]) is not None and r not in early]
        drops = [r for r in sel if i(r["turn_deep_before"]) is not None and i(r["turn_deep_after"]) is not None
                 and i(r["turn_deep_before"]) - i(r["turn_deep_after"]) >= 100]
        hor = sum(1 for r in drops if ingame(r) is not None and ingame(r) > -50)
        evm = sum(1 for r in drops if evals.get(id(r), (None, -1))[1] is not None and evals[id(r)][1] >= 0)
        print("| %s | %s | %d | %s | %d | %s | %d | %d / %d |" % (opp, net, len(sel), statistics.median(proven), len(early), bins(turn),
                                                         len(drops), hor, evm))
allp = [int(r["pieces_before"]) for r in ok]
alle = [r for r in ok if (i(r["turn_deep_before"]) or 0) < -50]
allt = [int(r["turn_pieces"]) for r in ok if i(r["turn_pieces"]) is not None and r not in alle]
print("\nAll %d analysed games: proven lost at median %s pieces; turns at 20–24 / 17–19 / 13–16 / 9–12 / ≤ 8 pieces: %s; "
      "unresolved or not proven: %d; already ≤ −50 at Regicide's first decision: %d." % (len(ok), statistics.median(allp), bins(allt), len(bad), len(alle)))
