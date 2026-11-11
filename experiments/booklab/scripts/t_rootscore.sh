L=~/regicide-jobs/booklab
MV=$(python3 -c "
import sys; sys.path.insert(0, '/mnt/c/regicide-match/jobs/booklab/src/tools/booklab')
from theory import parse_pdn
for t, m in parse_pdn(open('/home/admingod/regicide-match/results/bl-mo1-kingsrow/games.pdn').read()):
    if t['Round'] == '5.1': print(' '.join(x for x, _ in m[:50]))")
printf "t6\t\t%s\t25-22,8-11\nbad\t\t9-13 22-18 99-1\t\n" "$MV" | $L/bin/blk-rootscore-1c6e 4000000 2 ~/regicide-data/egdb 128
