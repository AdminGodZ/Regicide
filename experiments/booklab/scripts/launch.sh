#!/usr/bin/env bash
# launch.sh NAME SCRIPT [ARGS...]: run SCRIPT in its own wsl.exe session that lives as long as SCRIPT runs.
# (2026-11-11 00:08: Windows processes started through WSL interop - cbhost and the engines - die when the wsl.exe
# session that started their chain exits; a chain started with nohup/setsid from a short wsl call lost every game
# whose engines were running at that moment. A hidden Windows process "wsl.exe -e bash session.sh ..." keeps the
# session open until the script ends.) Arguments must not contain spaces (Start-Process joins them unquoted).
# Output: ~/regicide-jobs/booklab/NAME.out. Returns immediately.
NAME=$1; shift
for x in "$NAME" "$@"; do case "$x" in *" "*) echo "launch.sh: argument with a space: '$x'"; exit 2 ;; esac; done
list="'-e','bash','/mnt/c/regicide-match/jobs/booklab/session.sh','$NAME'"
for x in "$@"; do list="$list,'$x'"; done
/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe -NoProfile -Command \
  "Start-Process -WindowStyle Hidden -FilePath wsl.exe -ArgumentList $list" && echo "launched $NAME: $*"
