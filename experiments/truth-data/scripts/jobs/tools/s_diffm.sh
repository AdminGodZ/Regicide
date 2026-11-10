cd ~/regicide-truth
for f in tools/truth.cpp tools/datagen.cpp tools/nnue/nnue.py; do echo "=== $f"; git diff HEAD origin/master -- $f | grep -E "^[-+][^-+]" | head -12; done
