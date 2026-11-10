#!/usr/bin/env bash
# booklab: kept for the scripts that call it; one implementation in bl_run_w.sh (see its header: explicit
# "set book 3" with a pre-flight read-back for every engine, finished = exit 0 and all scheduled games played).
# Without MATCH/EXTRA_ARGS set it runs the final-validation runner match-747ab51, as this script always did.
exec bash /mnt/c/regicide-match/jobs/booklab/bl_run_w.sh "$@"
