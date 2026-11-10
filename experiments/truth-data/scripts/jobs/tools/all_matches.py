"""Every match of this experiment as one markdown table, with the engine build (the .so or DLL both sides or Regicide
used) and the commit it was built from, read from the match directories' summary.txt.

  python all_matches.py MATCHES_DIR
"""
import os
import re
import sys

# match directory name -> (engine file, commit it was built from); first match wins
BUILDS = [
    (r"^tr-.*-c0dde2e$", "DLL truth-c0dde2e", "c0dde2e (engine sources 9b4cfed: master 2efe11a merged)"),
    (r"^tr-", "DLL ponder-df7780a", "df7780a (ponder agent's build)"),
    (r"^ref-ponder-", "DLL ponder-df7780a", "df7780a (ponder agent's build and run)"),
    (r"^w\d+-", "rft2048.so (REGICIDE_NNUE_MAX_FT=2048)", "c0dde2e (engine sources 9b4cfed)"),
    (r"-nodb$", "rtruth.so without an egdb folder", "fcf6acb"),
    (r"^n7[ab]-vs-n6c$|^n6d-vs-n6c$", "rff0a3d2.so", "ff0a3d2"),
    (r".", "rtruth.so", "fcf6acb"),
]


def build_of(name):
    for pat, so, commit in BUILDS:
        if re.search(pat, name):
            return so, commit


def main(root):
    print("| match | engine (both sides, or Regicide vs an opponent) | built from | time | games | A: W / D / L | "
          "pentanomial | Elo [95% CI] |")
    print("|---|---|---|---|---|---|---|---|")
    for d in sorted(os.listdir(root)):
        f = os.path.join(root, d, "summary.txt")
        if not os.path.exists(f):
            continue
        t = open(f).read()
        a = re.search(r"^A: \+(\d+) =(\d+) -(\d+)", t, re.M)
        p = re.search(r"pentanomial \[[^]]*\] = \[([^]]*)\]", t)
        e = re.search(r"^Elo \(pentanomial, pairs\): (\S+ \[[^]]*\])", t, re.M)
        tc = re.search(r"Time control: (\S+)/move", t)
        g = re.search(r"Games: (\d+) of", t)
        so, commit = build_of(d)
        print("| %s | %s | %s | %s | %s | +%s =%s -%s | [%s] | %s |" % (
            d, so, commit, tc.group(1) if tc else "?", g.group(1) if g else "?", a.group(1), a.group(2), a.group(3),
            p.group(1) if p else "", e.group(1) if e else ""))


if __name__ == "__main__":
    main(sys.argv[1])
