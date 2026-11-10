"""Applies the audit corrections and the phase-2 additions to experiments/truth-data/RESULTS.md (in place).
Texts come from tools/res/*.md. Each replacement must apply exactly once (asserted)."""
import os
import re
import subprocess

R = os.path.expanduser("~/regicide-truth/experiments/truth-data/RESULTS.md")
RES = "/mnt/c/regicide-match/jobs/truth/tools/res"
t = open(R).read()


def res(name):
    return open(os.path.join(RES, name)).read().strip() + "\n"


def sub(pattern, repl, flags=re.S):
    global t
    t, n = re.subn(pattern, lambda m: repl, t, flags=flags)
    assert n == 1, (pattern[:70], n)


# status + evidence
sub(r"\*\*Status \(2026-11-10 10:05\):\*\*.*?\n\n(?=## E0: census)", res("status.md") + "\n" + res("evidence.md") + "\n")
# pilot paragraph (audit 2)
sub(r"it disagrees with the proof in 17\.5% of proven 9–14-piece positions\n  of the 10k files",
    "it disagrees with the proof in 18.5% of proven 9–14-piece positions\n  of the 10k files (17.5% over all files)")
# confirmation: same openings (audit 3)
sub(r"(\*\*n8bn is confirmed: \+4\.5 \[\+1\.6, \+7\.4\] at 0\.5 s/move\*\*, the same size as at 0\.1 s \(\+4\.5 \[\+0\.5, \+8\.6\]\)\.)",
    "**n8bn is confirmed: +4.5 [+1.6, +7.4] at 0.5 s/move**, the same size as at 0.1 s (+4.5 [+0.5, +8.6]). This is a "
    "second measurement at another time control on the **same 805 openings**, not a new opening sample (see \"Reading "
    "the evidence\").")
# P2 (audit 4)
sub(r"\*\*E3 does not add to E2\.\*\* n8c scores \+0\.4, below n8bn's \+4\.5, so the n8c–n8bn match was not played \(rule: only if\nn8c's point estimate beat \+4\.5\)\.",
    "**No gain from E3 over E2 shown.** n8c scores +0.4 against n8bn's +4.5 vs n6c; through n6c this comparison\n"
    "resolves only about 8 Elo. The direct match n8c vs n8bn (805 pairs) was not played here; the lead moved it to the\n"
    "Arch laptop (origin/t3/search-lab, \"Net matches (for truth-data)\").")
# P3 bullets (audit 4)
sub(r"- K: the trend upwards stops at 102\.4 \(K 130 \+3\.2\)\.",
    "- K: no gain beyond 102.4 shown (K 130 +3.2).")
sub(r"- λ: 0\.6 and 0\.7 do not improve; 0\.6 is the weakest of all, so λ around 0\.5 stays, with no clear trend\.\n- Phase-balanced sampling: equal to n8bn head to head in both splits\.",
    "- λ: 0.6 and 0.7 show no gain over 0.5 (0.6 has the lowest point estimate, −0.2, inside the others' intervals).\n"
    "- Phase-balanced sampling: no difference from n8bn shown head to head, in either split.\n"
    "- **Resolution:** an 805-pair test detects about 5–6 Elo, and a comparison through two results against n6c about\n"
    "  8 Elo. None of these differences reaches that, so all read \"no difference shown at this resolution\".")
# opponents (audit 4)
sub(r"and the nets do not differ \(16–19\n  wins, 2–3 losses\)\.",
    "and no difference between the nets is shown\n  (16–19 wins, 2–3 losses; these 312-game runs resolve about 6–17 Elo).")
# empty placeholders
t = t.replace("<!-- E3NETS -->\n", "").replace("<!-- E3MATCHES -->\n", "")
# T4 + P5 before the conclusions; conclusions replaced
sanity = open(os.path.expanduser("~/regicide-jobs/truth/sanity_n8q.md")).read().strip()
t4 = res("t4.md").replace("<!-- SANITY -->", sanity)
sub(r"<!-- P5 -->\n", t4 + "\n" + res("p5.md"))
sub(r"## What holds and what does not\n.*?(?=## Running in the background)", res("conclusions.md") + "\n")
# background + all matches
allm = subprocess.run(["python3", os.path.expanduser("~/regicide-truth/experiments/truth-data/scripts/all_matches.py"),
                       os.path.expanduser("~/regicide-truth/experiments/truth-data/matches")],
                      capture_output=True, text=True, check=True).stdout.strip()
sub(r"## Running in the background, and how to resume\n.*?(?=## Files)", res("background.md").replace("<!-- ALLMATCHES -->", allm) + "\n")
open(R, "w").write(t)
print("applied; %d lines" % t.count("\n"))
