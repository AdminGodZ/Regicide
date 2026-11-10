cd ~/regicide-truth
for f in tools/truth.cpp tools/datagen.cpp tools/nnue/nnue.py; do echo "=== $f (lines only in master)"; git diff HEAD origin/master -- $f | grep -E "^\+[^+]"; done
