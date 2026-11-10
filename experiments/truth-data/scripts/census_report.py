"""Markdown tables from `truth census` output (census.txt)."""
import re
import sys


def main(path):
    files = {}
    for line in open(path):
        w = line.split()
        if not w or w[0].startswith("#") or w[0] == "boundaries":
            continue
        name = w[1].replace(".bin", "")
        d = files.setdefault(name, {})
        if w[0] == "file":
            d["records"] = int(w[3])
            d["games"] = int(w[5])
            d["avg_len"] = float(w[7])
            d["le8"] = int(w[9])
        elif w[0] == "hist":
            d["hist"] = [int(x) for x in w[2:]]
        elif w[0] == "games":
            d["reach"] = int(w[3])
            d["disagree"] = int(w[8])
            d["changed"] = int(w[11])
        elif w[0] == "matrix":
            nums = [int(x) for x in re.findall(r"-?\d+", line.split(":", 1)[1])]
            d["m"] = [nums[0:3], nums[3:6], nums[6:9]]

    print("| File | Records | Games | ≤ 8 pieces | Games reaching ≤ 8 | Result ≠ database | of which db decisive, "
          "game drawn | db draw, game decisive | db and game opposite | ≥ 9-piece records relabelled |")
    print("|---|---|---|---|---|---|---|---|---|---|")
    tot = {"records": 0, "le8": 0, "games": 0, "reach": 0, "disagree": 0, "changed": 0, "ge9": 0,
           "dd": 0, "draw_dec": 0, "opp": 0}
    for name, d in files.items():
        m = d["m"]  # [db L/D/W][stored L/D/W]
        dec_draw = m[0][1] + m[2][1]
        draw_dec = m[1][0] + m[1][2]
        opp = m[0][2] + m[2][0]
        ge9 = d["records"] - d["le8"]
        print("| %s | %s | %s | %.1f%% | %.1f%% | **%.2f%%** | %d | %d | %d | %.2f%% |" % (
            name, f"{d['records']:,}", f"{d['games']:,}", 100 * d["le8"] / d["records"],
            100 * d["reach"] / d["games"], 100 * d["disagree"] / d["reach"], dec_draw, draw_dec, opp,
            100 * d["changed"] / ge9))
        if name.startswith("gen4b"):
            continue  # not training data
        for k in ("records", "le8", "games", "reach", "disagree", "changed"):
            tot[k] += d[k]
        tot["ge9"] += ge9
    print("| **training files (all but gen4b)** | %s | %s | %.1f%% | %.1f%% | %.2f%% | | | | %.2f%% |" % (
        f"{tot['records']:,}", f"{tot['games']:,}", 100 * tot["le8"] / tot["records"], 100 * tot["reach"] / tot["games"],
        100 * tot["disagree"] / tot["reach"], 100 * tot["changed"] / tot["ge9"]))

    print("\nPiece-count histogram (share of records, %):\n")
    print("| File | " + " | ".join(str(p) for p in range(2, 25)) + " |")
    print("|---|" + "---|" * 23)
    for name, d in files.items():
        h = d["hist"]
        n = sum(h)
        print("| %s | " % name + " | ".join("%.1f" % (100 * h[p - 1] / n) for p in range(2, 25)) + " |")


if __name__ == "__main__":
    main(sys.argv[1])
