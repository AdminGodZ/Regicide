"""Item 4b addition: the five games n8q lost to Cake with the new book (Book Lab, bl-f4-cake-h1/h2) against the same
ballots drawn with n6c (bl-f1-cake-a/b). At the first move where n8q's game leaves n6c's: both nets' static evals and
search scores of both candidate moves, at the match budget (the node count Regicide used in that game at that move, from
its PDN comment) and at 10x. Searches: the regicide CLI of this branch (master 456e9b2+ engine), set evalfile, the
8-piece database, book off, fresh hash per search; a move's score = -(score of the position after it)."""
import os
import re
import subprocess
import sys

R = os.path.expanduser("~/regicide-match/results")
E = os.path.expanduser("~/regicide-truth/build/regicide")
NETS = {"n8q": os.path.expanduser("~/regicide-truth-work/n8q.nnue"),
        "n6c": os.path.expanduser("~/regicide-nnue6-work/n6c.nnue")}
LOST = [("bl-f4-cake-h1", "33.1", "ACF #37", "9-14 22-17 6-9"), ("bl-f4-cake-h1", "67.1", "ACF #75", "10-14 24-19 6-10"),
        ("bl-f4-cake-h1", "78.1", "ACF #87", "10-15 21-17 9-13"), ("bl-f4-cake-h2", "33.1", "ACF #124", "11-15 23-18 12-16"),
        ("bl-f4-cake-h2", "40.2", "ACF #131", "11-15 24-20 12-16")]
REFS = ["bl-f1-cake-a-h1", "bl-f1-cake-a-h2", "bl-f1-cake-b-h1", "bl-f1-cake-b-h2"]


def games(path):
    text = open(path).read()
    for chunk in text.split("[Event ")[1:]:
        tags = dict(re.findall(r'^\[?(\w+) "([^"]*)"\]', "[Event " + chunk, re.M))
        body = re.sub(r"^\[.*\]$", "", chunk, flags=re.M)
        body = body[body.find("\n"):]
        moves, comments = [], []
        for tok in re.finditer(r"\{([^}]*)\}|(\S+)", body):
            if tok.group(1) is not None:
                if comments:
                    comments[-1] += tok.group(1)
                continue
            t = re.sub(r"^\d+\.+", "", tok.group(2))
            if not t or t in ("1-0", "0-1", "1/2-1/2", "*"):
                continue
            moves.append(t)
            comments.append("")
        yield tags, moves, comments


def regicide_colour(tags):
    return "B" if tags.get("Black", "").startswith("Regicide") else "W"


def session(cmds):
    out = subprocess.run([E], input="\n".join(cmds) + "\nquit\n", capture_output=True, text=True).stdout
    return out


def main():
    rows = []
    for run, rnd, ballot, bmoves in LOST:
        lost = next((g for g in games(os.path.join(R, run, "games.pdn")) if g[0].get("Round") == rnd), None)
        assert lost, (run, rnd)
        tags, moves, comments = lost
        col = regicide_colour(tags)
        assert tags.get("Opening", "").rsplit(": ", 1)[-1].strip() == bmoves, (ballot, tags.get("Opening"))
        ref = None
        for rr in REFS:
            for g in games(os.path.join(R, rr, "games.pdn")):
                if g[0].get("Opening", "").rsplit(": ", 1)[-1].strip() == bmoves and regicide_colour(g[0]) == col:
                    ref = (rr, g)
                    break
            if ref:
                break
        assert ref, ballot
        rmoves, rcomments = ref[1][1], ref[1][2]
        i = next(k for k in range(min(len(moves), len(rmoves))) if moves[k] != rmoves[k])
        nodes = {}
        for net, cm in (("n8q", comments[i]), ("n6c", rcomments[i])):
            m = re.search(r"nodes (\d+)", cm)
            nodes[net] = int(m.group(1)) if m else 1000000
        prefix = moves[:i]
        cand = {"n8q": moves[i], "n6c": rmoves[i]}
        res = {"ballot": ballot, "colour": col, "ply": i + 1, "after_ballot": i + 1 - 3, "n8q_move": cand["n8q"],
               "n6c_move": cand["n6c"], "result_ref": ref[1][0].get("Result"), "ref_run": ref[0],
               "nodes": nodes, "pieces": None}
        for net, path in NETS.items():
            cmds = ["set book off", "set egdb " + os.path.expanduser("~/regicide-data/egdb"), "set evalfile " + path,
                    "position startpos moves " + " ".join(prefix), "eval", "d"]
            for who in ("n8q", "n6c"):
                cmds += ["position startpos moves " + " ".join(prefix + [cand[who]]), "eval"]
            budgets = [nodes[net], 10 * nodes[net]]
            for b in budgets:
                for who in ("n8q", "n6c"):
                    cmds += ["newgame", "position startpos moves " + " ".join(prefix + [cand[who]]), "go nodes %d" % b]
            out = session(cmds)
            evals = [int(x) for x in re.findall(r"^eval (-?\d+)", out, re.M)]
            fen = re.findall(r"^([BW]:W[^ \n]*:B[^ \n]*)$", out, re.M)
            if fen:
                res["fen"] = fen[0]
                res["pieces"] = len(re.findall(r"\d+", fen[0].split(":", 1)[1]))
            blocks = out.split("bestmove")[:-1]
            scores = []
            for blk in blocks:
                s = re.findall(r"info depth (\d+) .*?score (-?\d+)", blk)
                scores.append((int(s[-1][0]), -int(s[-1][1])) if s else (0, None))  # negate: the mover's view
            res[net] = {"static_before": evals[0], "static_after": {"n8q": -evals[1], "n6c": -evals[2]},
                        "search": {"x1": {"n8q": scores[0], "n6c": scores[1]}, "x10": {"n8q": scores[2], "n6c": scores[3]}},
                        "budgets": budgets}
        rows.append(res)
        print(res, flush=True)
    # markdown
    print("\n| ballot (Regicide) | ply (after ballot) | pieces | n8q's move / n6c's move | net | static eval before | static after n8q's move / n6c's move | search ×1 (nodes): n8q's / n6c's move | search ×10: n8q's / n6c's move |")
    print("|---|---|---|---|---|---|---|---|---|")
    for r in rows:
        for net in ("n8q", "n6c"):
            x = r[net]
            f = lambda d: "%+d (d%d) / %+d (d%d)" % (d["n8q"][1], d["n8q"][0], d["n6c"][1], d["n6c"][0])
            print("| %s (%s) | %d (%d) | %s | %s / %s | %s | %+d | %+d / %+d | %s (%dk) | %s (%dk) |" % (
                r["ballot"], r["colour"], r["ply"], r["after_ballot"], r["pieces"], r["n8q_move"], r["n6c_move"], net,
                x["static_before"], x["static_after"]["n8q"], x["static_after"]["n6c"], f(x["search"]["x1"]),
                x["budgets"][0] // 1000, f(x["search"]["x10"]), x["budgets"][1] // 1000))


if __name__ == "__main__":
    main()
