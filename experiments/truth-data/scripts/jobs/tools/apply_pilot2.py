import os, re
R = os.path.expanduser("~/regicide-truth/experiments/truth-data/RESULTS.md")
t = open(R).read()
new = open("/mnt/c/regicide-match/jobs/truth/tools/res/pilot2.md").read().strip() + "\n"
t2, n = re.subn(r"> \*\*Note \(2026-11-10\)\.\*\*.*?\n(?!>)", lambda m: new, t, flags=re.S)
assert n == 1, n
open(R, "w").write(t2); print("ok")
