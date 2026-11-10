#!/usr/bin/env bash
# session.sh NAME SCRIPT [ARGS...]: body of a launch.sh session (stdout/stderr to ~/regicide-jobs/booklab/NAME.out)
NAME=$1; shift
exec > ~/regicide-jobs/booklab/$NAME.out 2>&1 < /dev/null
echo "session $NAME start $(date -Is): $*"
bash "$@"
echo "session $NAME end $(date -Is), exit $?"
