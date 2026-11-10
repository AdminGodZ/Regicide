#!/usr/bin/env python3
"""WP12: choose and score the candidates (protocol: experiments/booklab/wp12/PROTOCOL.md).

  wp12_select.py --points points.tsv --moves moves0.tsv --decisions decisions.txt --multiscore blk-multiscore
                 --egdb DIR --book book_r11.bin [--threads 6] [--nodes 20000000] [--slots 1] [--out DIR]

Inputs: points.tsv (wp12_points.py: our move positions on the r11 lines of the final-validation games against
Cake, book consulted, the book plays the played move), moves0.tsv (booklab_multiscore NODES=0 with book r11: the
legal moves of every point and their avoid flags), decisions.txt (r10's decisions: protected, never a candidate).

Rules fixed before any score is seen:
  - A point qualifies when it is not a protected decision position and has at least two legal moves not on the
    avoid list. (Before 8 pieces, book consulted, on the r11 line: wp12_points.py.)
  - Every legal move of a scored point is searched with booklab_multiscore: book off, cleared hash per move,
    NODES nodes (20 M), the score of the last completed iteration. The alternative is the best non-avoided move
    other than the pinned one; the point gives a candidate when that move's score >= pinned score - 15 (centi-men).
    No completed score for the pinned move or for every other move: no candidate.
  - Lines are (ballot, colour). Inside a line the points are ordered: category 2 (10..25 plies after Cake's book
    ends) by distance ascending, then category 3 (earlier) by distance descending (nearest to the end of Cake's
    book first). Category 4 (later) is not used.
  - Slots: a line gives at most three candidates, at least four plies apart (plies of the game the point was
    met in, from the start position). Slot s of a line is the first point in the line order after slot s-1's point
    that passes the margin and keeps the spacing; at most 6 points are scored per line and slot (the rest of the
    line is left for the next slot).
  - Global order: (1) the w30/w31 point (ballots 163/165, Black, ply 4, alternative 8-12; its own arm when it fails
    the margin); then slot 1 of every line in deck order (ballot number, Black before White), then slot 2, slot 3.
    The pilot is the first 300 candidates of this order.
Scores are cached in OUT/scores.tsv (appended; a restart reuses them). Outputs: OUT/considered.tsv (every scored
point with all moves, kept or not, and why), OUT/candidates.tsv (global order).
"""
import argparse
import collections
import os
import re
import subprocess
import sys

MARGIN = 15
PER_SLOT_SCAN = 6
W30_FEN = "B:W17,21,22,23,24,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,9,10,11,20"
W30_ALT = "8-12"


def ballot_no(b):
    m = re.search(r"#(\d+)", b)
    return int(m.group(1)) if m else 0


def main():
    ap = argparse.ArgumentParser()
    for k in ("points", "moves", "decisions", "multiscore", "egdb", "book"):
        ap.add_argument("--" + k, required=True)
    ap.add_argument("--threads", type=int, default=6)
    ap.add_argument("--nodes", type=int, default=20_000_000)
    ap.add_argument("--slots", type=int, default=1)
    ap.add_argument("--out", default=".")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)

    protected = set()
    for l in open(a.decisions):
        if l.startswith("#") or "|" not in l:
            continue
        protected.add(l.split("|")[0].strip())
    legal = collections.defaultdict(list)  # id -> [(move, avoided)]
    for l in open(a.moves):
        f = l.rstrip("\n").split("\t")
        legal[f[0]].append((f[1], f[6] == "1"))
    pts, lines = {}, collections.defaultdict(list)
    hdr = None
    for l in open(a.points):
        f = l.rstrip("\n").split("\t")
        if hdr is None:
            hdr = f
            continue
        p = dict(zip(hdr, f))
        p["category"], p["distance"], p["ply"] = int(p["category"]), int(p["distance"]), int(p["ply"])
        pts[p["id"]] = p
        if p["category"] in (2, 3):
            lines[(p["ballot"], p["colour"])].append(p)
    w30 = next(p for p in pts.values() if p["fen"] == W30_FEN)

    def qualifies(p):
        if p["fen"] in protected:
            return "protected decision"
        if sum(1 for _, av in legal[p["id"]] if not av) < 2:
            return "fewer than two non-avoided moves"
        return None

    for k in lines:
        lines[k].sort(key=lambda p: (0, p["distance"], p["ply"]) if p["category"] == 2 else (1, -p["distance"], p["ply"]))
    order = sorted(lines, key=lambda k: (ballot_no(k[0]), k[1] != "B"))

    cache_path = os.path.join(a.out, "scores.tsv")
    scores = collections.defaultdict(dict)  # id -> move -> (score|None, depth, nodes, partial)
    if os.path.exists(cache_path):
        for l in open(cache_path):
            f = l.rstrip("\n").split("\t")
            scores[f[0]][f[1]] = (None if f[2] == "none" else int(f[2]), int(f[3]), int(f[4]), int(f[5]))

    def score(batch):
        todo = [p for p in batch if p["id"] not in scores]
        if not todo:
            return
        inp = "".join(f"{p['id']}\t{p['fen']}\n" for p in todo)
        r = subprocess.run([a.multiscore, str(a.nodes), str(a.threads), a.egdb, "128", a.book], input=inp,
                           capture_output=True, text=True, check=True)
        with open(cache_path, "a") as fo:
            for l in r.stdout.splitlines():
                f = l.split("\t")
                if f[1] == "ERR":
                    sys.exit(f"multiscore: {l}")
                scores[f[0]][f[1]] = (None if f[2] == "none" else int(f[2]), int(f[3]), int(f[4]), int(f[5]))
                fo.write("\t".join(f[:6]) + "\n")
        print(f"scored {len(todo)} points ({sum(len(scores[p['id']]) for p in todo)} moves)", flush=True)

    def verdict(p, alt_forced=None):
        """(alternative, its score, pinned score, passes, why)"""
        sc = scores[p["id"]]
        ps = sc.get(p["pinned"], (None,))[0]
        if ps is None:
            return None, None, None, False, "no completed score for the pinned move"
        others = [(m, sc[m][0]) for m, av in legal[p["id"]] if not av and m != p["pinned"] and sc[m][0] is not None]
        if alt_forced:
            others = [(m, s) for m, s in others if m == alt_forced]
        if not others:
            return None, None, ps, False, "no completed score for any alternative"
        best = max(others, key=lambda ms: ms[1])
        ok = best[1] >= ps - MARGIN
        return best[0], best[1], ps, ok, "" if ok else f"alternative {best[1] - ps:+d} below the pinned move"

    considered = {}  # id -> row
    cands = []

    # (1) w30/w31
    score([w30])
    alt, alts, ps, ok, why = verdict(w30, W30_ALT)
    best_alt = verdict(w30)[0]
    considered[w30["id"]] = (w30, alt, alts, ps, ok, why + (f"; best other move by search: {best_alt}"))
    cands.append(dict(arm="w30/w31" if ok else "w30/w31 (own arm: fails the margin)", slot=0, p=w30, alt=alt,
                      alts=alts, ps=ps))

    # (2)/(3) slots
    state = {k: dict(pos=0, kept=[]) for k in order}
    for s in range(1, a.slots + 1):
        need = {k: dict(scanned=0, done=False) for k in order}
        while True:
            batch, want = [], {}
            for k in order:
                st, nd = state[k], need[k]
                if nd["done"]:
                    continue
                L = lines[k]
                while st["pos"] < len(L) and nd["scanned"] < PER_SLOT_SCAN:
                    p = L[st["pos"]]
                    if p["id"] == w30["id"]:
                        st["pos"] += 1
                        continue
                    if any(abs(p["ply"] - q["ply"]) < 4 for q in st["kept"]):
                        considered.setdefault(p["id"], (p, None, None, None, False, "within 4 plies of a kept point"))
                        st["pos"] += 1
                        continue
                    why = qualifies(p)
                    if why:
                        considered.setdefault(p["id"], (p, None, None, None, False, why))
                        st["pos"] += 1
                        continue
                    break
                if st["pos"] >= len(L) or nd["scanned"] >= PER_SLOT_SCAN:
                    nd["done"] = True
                    continue
                want[k] = L[st["pos"]]
                batch.append(L[st["pos"]])
            if not batch:
                break
            score(batch)
            for k, p in want.items():
                st, nd = state[k], need[k]
                alt, alts, ps, ok, why = verdict(p)
                considered[p["id"]] = (p, alt, alts, ps, ok, why)
                st["pos"] += 1
                nd["scanned"] += 1
                if ok:
                    st["kept"].append(p)
                    cands.append(dict(arm=f"slot {s}", slot=s, p=p, alt=alt, alts=alts, ps=ps))
                    nd["done"] = True
        write(a, cands, considered, scores, legal)
        print(f"slot {s}: {sum(1 for c in cands if c['slot'] == s)} candidates", flush=True)
    write(a, cands, considered, scores, legal)


def write(a, cands, considered, scores, legal):
    with open(os.path.join(a.out, "candidates.tsv"), "w") as fo:
        fo.write("rank\tarm\tpoint\tcategory\tdistance\tballot\tcolour\trun\tround\tply\tfen\tpinned\tpinned_score\t"
                 "alternative\talt_score\tother_lines\n")
        for i, c in enumerate(cands):
            p = c["p"]
            fo.write(f"{i}\t{c['arm']}\t{p['id']}\t{p['category']}\t{p['distance']}\t{p['ballot']}\t{p['colour']}\t"
                     f"{p['run']}\t{p['round']}\t{p['ply']}\t{p['fen']}\t{p['pinned']}\t{c['ps']}\t{c['alt']}\t"
                     f"{c['alts']}\t{p['other_lines']}\n")
    with open(os.path.join(a.out, "considered.tsv"), "w") as fo:
        fo.write("point\tcategory\tdistance\tballot\tcolour\tply\tfen\tpinned\tpinned_score\talternative\talt_score\t"
                 "kept\twhy\tmoves (move:score/depth/partial, * avoided)\n")
        for pid, (p, alt, alts, ps, ok, why) in considered.items():
            sc = scores.get(pid, {})
            mv = " ".join(f"{m}{'*' if av else ''}:{sc[m][0] if m in sc else '-'}/{sc[m][1] if m in sc else '-'}"
                          f"/{sc[m][3] if m in sc else '-'}" for m, av in legal[pid]) if sc else ""
            fo.write(f"{pid}\t{p['category']}\t{p['distance']}\t{p['ballot']}\t{p['colour']}\t{p['ply']}\t{p['fen']}\t"
                     f"{p['pinned']}\t{ps}\t{alt}\t{alts}\t{int(bool(ok))}\t{why}\t{mv}\n")


if __name__ == "__main__":
    main()
