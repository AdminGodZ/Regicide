cd ~/regicide-truth
for c in 769ef0b ff0a3d2; do echo "$c net.bin $(git show $c:src/weights/net.bin 2>/dev/null | sha256sum | cut -c1-16)"; done
for n in ~/regicide-nnue4-work/n4c.nnue ~/regicide-nnue6-work/n6c.nnue; do echo "$(basename $n) $(sha256sum $n | cut -c1-16)"; done
grep -h "^BA start tm-off\|^BA start ba-tm" ~/regicide-jobs/r4/status | cut -c1-120 | head -2
grep -h "tm-off" ~/regicide-jobs/r4/status | head -2 | cut -c1-140
