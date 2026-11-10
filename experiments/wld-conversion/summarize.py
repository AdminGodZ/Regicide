#!/usr/bin/env python3
"""Markdown tables from the conversion_test logs in logs/before and logs/after (run.sh, run_real.sh)."""
import pathlib
import re
import sys

root = pathlib.Path(__file__).parent / "logs"
head = re.compile(r"^(convert|defend) (\d) pieces, (\d+) positions, (\d+) nodes/move, MTC cap (\d+): (.*)$")


def parse(path):
    rows, lines = [], path.read_text().splitlines()
    for i, line in enumerate(lines):
        m = head.match(line)
        if not m:
            continue
        role, pieces, n, nodes, cap, rest = m.groups()
        counts = dict((k.strip(), int(v)) for k, v in re.findall(r"([a-z ]+?) (\d+),", rest))
        mean = re.search(r"mean plies: engine ([\d.]+), reference ([\d.]+)", rest)
        kinds = re.findall(r"(kings only|men on one side|men on both sides) (\d+)/(\d+)", lines[i + 1])
        stats = re.search(r"engine minus optimal: median (-?\d+) p90 (-?\d+) max (-?\d+)", lines[i + 2])
        rows.append(dict(role=role, pieces=int(pieces), n=int(n), nodes=int(nodes), cap=int(cap), counts=counts,
                         engine=float(mean.group(1)), ref=float(mean.group(2)),
                         kinds={k: (int(w), int(t)) for k, w, t in kinds},
                         excess=stats.groups() if stats else ("", "", "")))
    return rows


def table(prefix, role):
    print(f"| test | pieces | MTC cap | nodes/move | before: won | after: won | after failures | "
          f"mean plies before / after / reference | after: engine - reference (median, p90, max) | "
          f"by kind before → after (kings / men one side / men both sides) |")
    print("|---|---|---|---|---|---|---|---|---|---|")
    for f in sorted((root / "after").glob(prefix + "*.txt"), key=lambda p: (p.name.split("_")[:-1], int(p.stem.split("_")[-1]))):
        after = [r for r in parse(f) if r["role"] == role]
        before_file = root / "before" / f.name
        before = [r for r in parse(before_file) if r["role"] == role] if before_file.exists() else []
        for a in after:
            b = next((r for r in before if r["pieces"] == a["pieces"]), None)
            won = a["counts"].get("won", 0)
            fails = ", ".join(f"{k} {v}" for k, v in a["counts"].items() if k not in ("won", "reference game not won"))
            names = ("kings only", "men on one side", "men on both sides")
            kinds = " / ".join(
                (f"{b['kinds'].get(k, (0, 0))[0]}→" if b else "") + f"{w}/{t}" if t else "-"
                for k, (w, t) in ((k, a["kinds"].get(k, (0, 0))) for k in names))
            bw = f"{b['counts'].get('won', 0)}/{b['n']}" if b else "-"
            bm = f"{b['engine']:.1f}" if b else "-"
            print(f"| {f.stem.rsplit('_', 1)[0]} | {a['pieces']} | {a['cap']} | {a['nodes'] // 1000}k | {bw} | "
                  f"{won}/{a['n']} | {fails or '-'} | {bm} / {a['engine']:.1f} / {a['ref']:.1f} | "
                  f"{', '.join(a['excess'])} | {kinds} |")
    print()


for prefix, role in [(p, r) for p in sys.argv[1:] for r in ("convert", "defend")]:
    if any(r["role"] == role for f in (root / "after").glob(prefix + "*.txt") for r in parse(f)):
        print(f"### {prefix} ({role})\n")
        table(prefix, role)
