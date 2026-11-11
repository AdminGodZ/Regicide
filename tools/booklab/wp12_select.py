#!/usr/bin/env python3
"""WP12: choose and score the candidates (PROTOCOL.md on master e670910).

  wp12_select.py --points points.tsv --decisions decisions_r10.txt --multiscore blk-multiscore --net net.bin
                 --egdb DIR --book book_r11.bin [--threads 6] [--nodes 20000000] [--hash 128] [--slots 3]
                 --cache scores.tsv --out DIR

Campaign key (review M2): the SHA-256 of the scoring binary (booklab_multiscore: engine build, embedded net), of the
net file it embeds (--net, recorded separately), of the book used for the avoid flags, of the database directory's
manifest (file names and sizes), the node budget and the hash size. A cached score is used only when the campaign
key, the position (FEN) and the full list of legal moves are the same; the cache is a TSV (append only) shared by
campaigns. Header of every output: the campaign key and its parts.

Rules fixed before any score is seen:
  - A point qualifies when it is not a protected decision position (r10's decisions) and has at least two legal
    moves that are not on the avoid list (multiscore NODES=0 with the book). (Before 8 pieces, book consulted, on the
    r11 line: wp12_points.py.)
  - Every legal move of a scored point is searched with booklab_multiscore: book off, cleared hash per move, NODES
    nodes, the score of the last completed iteration. The alternative is the best non-avoided move other than the
    pinned one; the point gives a candidate when that move's score >= pinned score - 15 (centi-men). No completed
    score for the pinned move or for every other move: no candidate.
  - Lines are (ballot, colour) of the Cake games the points come from. Inside a line its points are ordered:
    category 2 (10..25 plies after Cake's book ends) by distance ascending, then category 3 (earlier) by distance
    descending. Category 4 is not used.
  - A line gives at most three candidates, at least four plies apart (ply of the point, from the start position). A
    candidate counts on every line whose Cake game passes through its position (its contexts against Cake in the
    point source: every supplied Cake validation game, any run or repeat), not only on the line it was found on.
  - The known position (w30/w31: ballots 163/165, Black, alternative 8-12) is scored first, if it qualifies like
    any point, the book does not already play 8-12 there and 8-12 is a legal non-avoided move (otherwise, or if
    either move has no completed score, it is no candidate and says why in considered.tsv). If it passes the margin
    it is candidate 0 and counts as one of the three on its lines (review M3, protocol); if it fails, it is its own
    arm and does not count.
  - Slot s of a line is the first point in the line order after slot s-1's point that passes the margin and keeps
    the spacing; at most 6 points are scored per line and slot. Global order: candidate 0, then slot 1 of every line
    in deck order (ballot number, Black before White), then slot 2, slot 3. The pilot is the first 300.
Outputs: OUT/considered.tsv (every point considered, kept or not, with all move scores and why), OUT/candidates.tsv
(global order, with each candidate's contexts), OUT/campaign.txt (the key), and the SHA-256 of candidates.tsv in
OUT/candidates.sha256 (the discovery ledger is bound to it).
"""
import argparse
import collections
import hashlib
import json
import os
import re
import subprocess
import sys

MARGIN = 15
PER_SLOT_SCAN = 6
W30_FEN = "B:W17,21,22,23,24,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,9,10,11,20"
W30_ALT = "8-12"


def sha(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def manifest_sha(d, out_path=None):
    """Content identity of the database directory (review 2026-11-11): SHA-256 of every file (sha256sum, 6 in
    parallel), the sorted 'name checksum' list written to out_path, and the SHA-256 of that list returned."""
    names = sorted(n for n in os.listdir(d) if os.path.isfile(os.path.join(d, n)))
    r = subprocess.run(["xargs", "-0", "-P", "6", "-n", "8", "sha256sum"], input="\0".join(names), cwd=d,
                       capture_output=True, text=True, check=True)
    rows = sorted(f"{l.split()[1].lstrip('*')} {l.split()[0]}" for l in r.stdout.splitlines() if l.strip())
    if len(rows) != len(names):
        sys.exit(f"database manifest: {len(rows)} checksums for {len(names)} files")
    text = "\n".join(rows) + "\n"
    if out_path:
        open(out_path, "w").write(text)
    return hashlib.sha256(text.encode()).hexdigest()


def ballot_no(b):
    m = re.search(r"#(\d+)", b)
    return int(m.group(1)) if m else 0


def main():
    ap = argparse.ArgumentParser()
    for k in ("points", "decisions", "multiscore", "net", "egdb", "book", "cache", "out"):
        ap.add_argument("--" + k, required=True)
    ap.add_argument("--threads", type=int, default=6)
    ap.add_argument("--nodes", type=int, default=20_000_000)
    ap.add_argument("--hash", type=int, default=128)
    ap.add_argument("--slots", type=int, default=3)
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    parts = dict(scorer=sha(a.multiscore), net=sha(a.net), book=sha(a.book),
                 egdb_content=manifest_sha(a.egdb, os.path.join(a.out, "egdb_manifest.txt")),
                 nodes=a.nodes, hash_mb=a.hash, margin=MARGIN)
    key = hashlib.sha256(json.dumps(parts, sort_keys=True).encode()).hexdigest()[:16]
    with open(os.path.join(a.out, "campaign.txt"), "w") as fo:
        fo.write(f"campaign {key}\n" + "".join(f"{k} {v}\n" for k, v in sorted(parts.items())))
    print(f"campaign {key}", flush=True)

    protected = {l.split("|")[0].strip() for l in open(a.decisions) if "|" in l and not l.startswith("#")}
    pts, lines, hdr = {}, collections.defaultdict(list), None
    for l in open(a.points):
        f = l.rstrip("\n").split("\t")
        if hdr is None:
            hdr = f
            continue
        p = dict(zip(hdr, f))
        p["category"], p["distance"], p["ply"] = int(p["category"]), int(p["distance"]), int(p["ply"])
        # every Cake line that reaches the position in any supplied validation game (every Cake context of the point,
        # from any run and repeat: review 2026-11-11 finding 1; earlier only contexts of the point's own run stem
        # counted), with the position's ply on that line; cap and spacing are checked on each of them with that ply
        p["line_ply"] = {(p["ballot"], p["colour"]): p["ply"]}
        for c in p["contexts"].split(";"):
            f5 = c.split("|")
            if len(f5) == 5 and f5[2] == "cake":
                ln = (f5[0], f5[1])
                p["line_ply"][ln] = min(p["line_ply"].get(ln, 10 ** 6), int(f5[4]))
        pts[p["id"]] = p
        if p["category"] in (2, 3):
            lines[(p["ballot"], p["colour"])].append(p)
    w30 = next((p for p in pts.values() if p["fen"] == W30_FEN), None)
    if w30 is None:
        sys.exit("the w30/w31 position is not among the points")

    # legal moves and avoid flags (no search)
    r = subprocess.run([a.multiscore, "0", "1", a.egdb, str(a.hash), a.book], capture_output=True, text=True,
                       check=True, input="".join(f"{p['id']}\t{p['fen']}\n" for p in pts.values()))
    legal = collections.defaultdict(list)
    for l in r.stdout.splitlines():
        f = l.split("\t")
        legal[f[0]].append((f[1], f[6] == "1"))
    movelist = {pid: ",".join(sorted(m for m, _ in mv)) for pid, mv in legal.items()}

    def qualifies(p):
        if p["fen"] in protected:
            return "protected decision"
        if sum(1 for _, av in legal[p["id"]] if not av) < 2:
            return "fewer than two non-avoided moves"
        return None

    for k in lines:
        lines[k].sort(key=lambda p: (0, p["distance"], p["ply"]) if p["category"] == 2 else (1, -p["distance"], p["ply"]))
    order = sorted(lines, key=lambda k: (ballot_no(k[0]), k[1] != "B"))

    # cache: campaign, fen, move list, move, score, depth, nodes, partial
    scores = collections.defaultdict(dict)
    by_fen = {p["fen"]: p["id"] for p in pts.values()}
    if os.path.exists(a.cache):
        for l in open(a.cache):
            f = l.rstrip("\n").split("\t")
            if len(f) != 8 or f[0] != key or f[1] not in by_fen or movelist[by_fen[f[1]]] != f[2]:
                continue
            scores[by_fen[f[1]]][f[3]] = (None if f[4] == "none" else int(f[4]), int(f[5]), int(f[6]), int(f[7]))
    for pid in list(scores):  # only complete move lists count as cached
        if set(scores[pid]) != {m for m, _ in legal[pid]}:
            del scores[pid]
    print(f"{len(scores)} points already scored in this campaign", flush=True)

    def score(batch):
        todo = [p for p in batch if p["id"] not in scores]
        if not todo:
            return
        r = subprocess.run([a.multiscore, str(a.nodes), str(a.threads), a.egdb, str(a.hash), a.book],
                           input="".join(f"{p['id']}\t{p['fen']}\n" for p in todo), capture_output=True, text=True,
                           check=True)
        got = collections.defaultdict(dict)
        for l in r.stdout.splitlines():
            f = l.split("\t")
            if f[1] == "ERR":
                sys.exit(f"multiscore: {l}")
            got[f[0]][f[1]] = (None if f[2] == "none" else int(f[2]), int(f[3]), int(f[4]), int(f[5]))
        with open(a.cache, "a") as fo:
            for p in todo:
                if set(got[p["id"]]) != {m for m, _ in legal[p["id"]]}:
                    sys.exit(f"multiscore returned an incomplete move list for {p['id']}")
                scores[p["id"]] = got[p["id"]]
                for m, (s, d, n, part) in got[p["id"]].items():
                    fo.write(f"{key}\t{p['fen']}\t{movelist[p['id']]}\t{m}\t{'none' if s is None else s}\t{d}\t{n}\t{part}\n")
        print(f"scored {len(todo)} points", flush=True)

    def verdict(p, alt_forced=None):
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

    considered, cands = {}, []
    kept = collections.defaultdict(list)  # line -> kept candidate points (counting on every Cake line through them)

    # kept[line] = [(point, its ply on that line)]
    def blocked(p):
        """Why p cannot be kept: on some line it reaches, three candidates are kept already, or one is within four
        plies (each line with its own ply). None if it can be kept."""
        for ln, ply in p["line_ply"].items():
            if len(kept[ln]) >= 3:
                return f"line {ln[0]} {ln[1]} has three candidates"
            if any(abs(ply - qply) < 4 for _, qply in kept[ln]):
                return f"within 4 plies of a kept candidate on line {ln[0]} {ln[1]}"
        return None

    def keep(p, arm, slot, alt, alts, ps):
        cands.append(dict(arm=arm, slot=slot, p=p, alt=alt, alts=alts, ps=ps))
        for ln, ply in p["line_ply"].items():
            kept[ln].append((p, ply))

    # the known position first (review 2026-11-11 finding 2: the same eligibility as every point; the alternative must
    # be a legal, non-avoided move other than the book's; a missing score is not a failed margin)
    nonav = {m for m, av in legal[w30["id"]] if not av}
    pre = qualifies(w30) or ("the book already plays " + W30_ALT if w30["pinned"] == W30_ALT else None) or \
        (None if W30_ALT in nonav else f"{W30_ALT} is not a legal non-avoided move there")
    if pre:
        considered[w30["id"]] = (w30, None, None, None, False, "known position not a candidate: " + pre)
    else:
        score([w30])
        alt, alts, ps, ok, why = verdict(w30, W30_ALT)
        best_other = verdict(w30)[0]
        considered[w30["id"]] = (w30, alt, alts, ps, ok, (why + "; " if why else "") + f"best other move by search: {best_other}")
        if ok:
            keep(w30, "w30/w31 (counts on its lines)", 0, alt, alts, ps)
        elif ps is None or alts is None:
            considered[w30["id"]] = (w30, alt, alts, ps, False, "known position not a candidate: " + why)
        else:
            cands.append(dict(arm="w30/w31 own arm (fails the margin; not counted)", slot=0, p=w30, alt=alt, alts=alts,
                              ps=ps))

    pos = {k: 0 for k in order}
    for s in range(1, a.slots + 1):
        scanned = {k: 0 for k in order}
        done = {k: len(kept[k]) >= 3 for k in order}
        while True:
            want = {}
            for k in order:
                if done[k]:
                    continue
                L = lines[k]
                while pos[k] < len(L) and scanned[k] < PER_SLOT_SCAN:
                    p = L[pos[k]]
                    reason = ("already the known position" if p["id"] == w30["id"] else
                              blocked(p) or qualifies(p))
                    if reason:
                        considered.setdefault(p["id"], (p, None, None, None, False, reason))
                        pos[k] += 1
                        continue
                    break
                if pos[k] >= len(L) or scanned[k] >= PER_SLOT_SCAN or len(kept[k]) >= 3:
                    done[k] = True
                    continue
                want[k] = L[pos[k]]
            if not want:
                break
            score(list({p["id"]: p for p in want.values()}.values()))
            for k, p in want.items():
                alt, alts, ps, ok, why = verdict(p)
                considered[p["id"]] = (p, alt, alts, ps, ok, why)
                pos[k] += 1
                scanned[k] += 1
                if ok:
                    why_not = blocked(p)  # a candidate kept on another line this round may have filled one of p's lines
                    if why_not:
                        considered[p["id"]] = (p, alt, alts, ps, False, "passes the margin, but " + why_not)
                    else:
                        keep(p, f"slot {s}", s, alt, alts, ps)
                        done[k] = True
        write(a, key, cands, considered, scores, legal)
        print(f"slot {s}: {sum(1 for c in cands if c['slot'] == s)} candidates", flush=True)
    write(a, key, cands, considered, scores, legal)


def write(a, key, cands, considered, scores, legal):
    path = os.path.join(a.out, "candidates.tsv")
    with open(path, "w") as fo:
        fo.write(f"# campaign {key}\n")
        fo.write("rank\tarm\tpoint\tcategory\tdistance\tballot\tcolour\trun\tround\tply\tfen\tpinned\tpinned_score\t"
                 "alternative\talt_score\tcontexts\n")
        for i, c in enumerate(cands):
            p = c["p"]
            fo.write(f"{i}\t{c['arm']}\t{p['id']}\t{p['category']}\t{p['distance']}\t{p['ballot']}\t{p['colour']}\t"
                     f"{p['run']}\t{p['round']}\t{p['ply']}\t{p['fen']}\t{p['pinned']}\t{c['ps']}\t{c['alt']}\t"
                     f"{c['alts']}\t{p['contexts']}\n")
    open(os.path.join(a.out, "candidates.sha256"), "w").write(sha(path) + "\n")
    with open(os.path.join(a.out, "considered.tsv"), "w") as fo:
        fo.write(f"# campaign {key}\n")
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
