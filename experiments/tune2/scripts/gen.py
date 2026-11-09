#!/usr/bin/env python3
"""Regenerate experiments/tune1/RESULTS.md from runner.log + queue.txt (+ notes.md appended verbatim)."""
import re, pathlib
T = pathlib.Path.home() / "regicide-jobs/tune2"
OUT = pathlib.Path("/home/admingod/Documents/Projects/Regicide-tune2/experiments/tune2/RESULTS.md")
DEF = dict(aspiration_delta=12, rfp_margin=70, rfp_max_depth=4, lmr_base=60, lmr_mult=45, lmr_min_depth=3,
           threat_qext=1, single_reply_ext=1, etc_min_depth=4, probcut_depth=99, probcut_margin=80,
           probcut_reduction=4, lmp_depth=3, lmp_base=3, fut_depth=3, fut_margin=60, cont_hist=1, se_depth=99,
           se_margin=40, razor_depth=0, razor_margin=150)
meta = {}
for line in (T / "queue.txt").read_text().splitlines():
    if line.strip() and not line.startswith("#"):
        f = line.split("|")
        meta[f[0]] = (f[1], int(f[2]), f[4] if len(f) > 4 else "", f[5] if len(f) > 5 else "")
rx = re.compile(r"^DONE (\S+) rc=(\d+) secs=(\d+) .*games (\d+)\s+A: \+(\d+) =(\d+) -(\d+).*elo ([-+0-9.]+) "
                r"\[([-+0-9.]+), ([-+0-9.]+)\]\s+penta \[([0-9 ]+)\]")
rows = {}
for line in (T / "runner.log").read_text().splitlines():
    m = rx.match(line)
    if not m: continue
    name, rc, secs, games, w, d, l, elo, lo, hi, pen = m.groups()
    a, mt, b, extra = meta[name]
    base = "t2"
    D = dict(DEF); D["se_depth"] = 4; D["se_margin"] = 20
    kv = [p.split("=") for p in a.split(",") if p]
    param = ", ".join(k for k, _ in kv)
    val = ", ".join(f"{v} ({D.get(k, '?')})" for k, v in kv)
    if b: val = f"{a} **vs** {b}"; param = "head-to-head"
    if extra: param = "Lazy SMP"; val = "`" + extra + "` (A threads vs B threads; games in parallel)"
    flag = " **✓**" if float(elo) >= 5 and float(lo) > 0 else ""
    row = f"| {param} | {val} | {w} / {d} / {l} | {elo} [{lo}, {hi}]{flag} | {pen} | {int(games)//2} | {int(secs)//60} min |"
    key = (base, "combo" if "combo" in name else mt)
    rows.setdefault(key, []).append(row)
H = "| param | value (default) | W / D / L | Elo [95% CI] | penta [LL LD DD/WL WD WW] | pairs | time |\n|---|---|---|---|---|---|---|"
s = (T / "header.md").read_text().rstrip() + "\n\n"
s += "✓ = at least +5 Elo with the CI lower bound above 0.\n\n"
def section(title, key):
    global s
    if rows.get(key):
        s += f"### {title}\n\n" + H + "\n" + "\n".join(rows[key]) + "\n\n"
s += "## Results\n\n"
section("Screen at 300 ms/move", ("t2", 300))
section("Confirmation at 1000 ms/move", ("t2", 1000))
section("Other", ("t2", 100))
notes = T / "notes.md"
if notes.exists():
    s += notes.read_text()
OUT.write_text(s)
print(s[-1500:])
