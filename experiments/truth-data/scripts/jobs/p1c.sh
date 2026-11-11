#!/usr/bin/env bash
# Reference for P1b: n6c (built-in) on the same conversion-search DLL (truth-c0dde2e), after n8c's runs.
source /mnt/c/regicide-match/jobs/truth/common.sh
until grep -q "^TR P1B_DONE" $J/status; do sleep 60; done
R=/mnt/c/regicide-match/jobs/truth/run_cb.sh; export DLLDIR=truth-c0dde2e
bash $R n6c-guinn-c0dde2e guinn builtin
bash $R n6c-cake-c0dde2e cake builtin
st "P1C_DONE"
