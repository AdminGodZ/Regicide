import os
t = open(os.path.expanduser("~/regicide-truth/experiments/truth-data/RESULTS.md")).read()
i = t.index("**Progress**"); print(repr(t[i:i+60])); print(repr(t[i+300:i+420]))
