#!/usr/bin/env python3
"""Panel H3: openings the strict check could neither clear nor prove touched.

build_holdout.py leaves 612 of the 805 four-ply openings unresolved (its bounded search hit the cap before it
could show that no forced book move is reachable). They are not certified clean, but no contact is known either,
and in practice a game from them rarely walks into one of the 975 changed book positions. H3 takes 156 of them
with the same seeded order as the other panels (ascending rank_sha256 in opening_audit.tsv), so the engines can be
compared on many fresh openings; whether a game actually met a changed book position is recorded after play.

Usage: python3 build_h3.py [--size 156]   (run in experiments/holdout; reads opening_audit.tsv)

Panel H4 (2026-11-11): python3 build_h3.py --skip 156 --name h4_unresolved156.txt
The next 156 openings in the same order. H3 stopped being a holdout for the book when its games against Cake (test
T1) went into the pin pool of r11b and later books: 3,965 pins of r11c come from H3 games. H4 has not been played.
Use it with the book off to compare engines, and with the book on only together with a count of the games that met
a book position.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path

here = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument("--size", type=int, default=156)
parser.add_argument("--skip", type=int, default=0, help="leave out the first N openings of the order (earlier panels)")
parser.add_argument("--name", default="h3_unresolved156.txt")
args = parser.parse_args()

rows = list(csv.DictReader(open(here / "opening_audit.tsv"), delimiter="\t"))
pool = sorted((r for r in rows if r["overall"] == "unresolved"), key=lambda r: r["rank_sha256"])
picked, seen = [], set()
for r in pool:
    if r["final_fen"] in seen:
        continue
    seen.add(r["final_fen"])
    picked.append(r)
    if len(picked) == args.skip + args.size:
        break
picked = picked[args.skip:]

manifest = json.load(open(here / "manifest.json"))
lines = ["# Regicide holdout panel: " + args.name,
         "# Actual openings: %d of %d unresolved; each is played with both colours." % (len(picked), len(pool)),
         "# Not certified clean: the strict contact check was capped. No contact with the book work is known.",
         "# seed=%d; order = ascending rank_sha256 of opening_audit.tsv; no static-evaluation filtering."
         % manifest["seed"]]
for r in picked:
    lines += ["# source openings4 line " + r["source_line"], r["moves"]]
text = "\n".join(lines) + "\n"
(here / args.name).write_text(text)
evals = sorted(int(r["eval_black"]) for r in picked)
print("%s: %d openings, sha256 %s" % (args.name, len(picked), hashlib.sha256(text.encode()).hexdigest()))
print("static eval (Black's view, n6c): min %d, median %d, max %d, mean %.2f"
      % (evals[0], evals[len(evals) // 2], evals[-1], sum(evals) / len(evals)))
