set -e
S=/mnt/c/regicide-match/jobs/truth/tools; T=~/regicide-truth
cp $S/datagen.cpp $T/tools/datagen.cpp; cp $S/dedupe_games.py $S/dupgames.py $T/experiments/truth-data/scripts/
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
cd $T && nice cmake --build build -j 3 2>&1 | grep -E " error|warning: " | grep -v store.cpp | head || true
rm -f /tmp/fixes/d.bin; ./build/datagen --games 3000 --nodes 300 --threads 2 --seed 4 --out /tmp/fixes/d.bin | grep "^games" | tail -n 1
~/regicide-nnue/.venv/bin/python $S/dupgames.py /tmp/fixes/d.bin | head -n 1
(cd build && ctest -j 4 2>&1 | grep "tests passed")
cp /mnt/c/regicide-match/jobs/truth/e5_gen.sh experiments/truth-data/scripts/jobs/
bash $S/commit_push.sh "truth-data review fixes (data pipeline): datagen seeds its streams with the records already in the output and refuses partial-record files; datagen uses each start position once per run (E5 had 19-23% repeated games); split carries game-start flags to the first retained record; validation tails of flagged files start at a game; e5_gen.sh fails on a failed binary copy and logs its checksum; dedupe and duplicate-check scripts" tools/datagen.cpp tools/truth.cpp tools/nnue/nnue.py experiments/truth-data/scripts
