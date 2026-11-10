import os
R = os.path.expanduser("~/regicide-truth/experiments/truth-data/RESULTS.md")
t = open(R).read()
head = open("/mnt/c/regicide-match/jobs/truth/tools/res/phase3_head.md").read()
tab = open(os.path.expanduser("~/regicide-jobs/truth/pilot_truth.md")).read().strip()
read = ("\n\n**Reading:**\n"
        "- All nets agree in sign with 99.3–99.95% of the proven positions; 10 and 12 pieces are the weakest counts.\n"
        "- The truth-data nets (n8bn, n8bn2, n8c, n8q) have about a third fewer weakly scored proofs than n6c (9–12 pieces:\n"
        "  625–680 against 947), which matches what the E2 labels teach.\n"
        "- n8q, with its 10% ≤ 8-piece share, is level with n8bn here.\n\n<!-- PHASE3MORE -->\n")
block = head + tab + read
i = t.index("## What holds and what does not")
if "## Phase 3" not in t:
    t = t[:i] + block + "\n" + t[i:]
open(R, "w").write(t); print("ok")
