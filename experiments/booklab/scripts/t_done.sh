# done_run of bl_run_w.sh on made-up summaries (sourced functions only)
T=$(mktemp -d); OUT=$T; DECK=/mnt/c/regicide-match/jobs/booklab/acf156_h1.txt; EXTRA_ARGS=""
eval "$(sed -n '/^bad_games()/p; /^expected_games() {/,/^}/p; /^played_games() {/,/^}/p; /^done_run() {/,/^}/p' /mnt/c/regicide-match/jobs/booklab/bl_run_w.sh)"
echo 0 > $T/exit_status; : > $T/games.pdn
printf "Games: 150 of 156 scheduled\nA: x\nUnfinished (move limit): 6\n" > $T/summary.txt; done_run && echo "150+6 of 156: done (ok)" || echo "150+6: FAIL"
printf "Games: 150 of 156 scheduled\nA: x\n" > $T/summary.txt; done_run && echo "150 of 156: FAIL" || echo "150 of 156: not done (ok)"
printf "Games: 156 of 156 scheduled\nA: x\n" > $T/summary.txt; done_run && echo "156 of 156: done (ok)" || echo "156: FAIL"
rm -rf $T
