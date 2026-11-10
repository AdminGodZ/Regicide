cd ~/regicide-truth
git diff 7be2f76^ 7be2f76 -- tools/truth.cpp > /tmp/7be.patch; wc -l < /tmp/7be.patch
git apply --3way /tmp/7be.patch && echo applied
grep -n "count == 0\|no complete record\|without a complete record" tools/truth.cpp | head
git fetch -q origin
echo "=== files I edit via copies: lines in master but not in HEAD (master side of the diff)"
for f in tools/truth.cpp tools/datagen.cpp tools/rescore.cpp tools/nnue/nnue.py tools/nnue/train.py tools/nnue/check_export.py src/datafile.hpp; do
  n=$(git diff HEAD origin/master -- $f | grep -c "^+[^+]"); echo "$f: $n lines only in master"
done
