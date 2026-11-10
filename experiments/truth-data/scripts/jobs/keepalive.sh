#!/usr/bin/env bash
# Keeps the WSL VM up while ~/regicide-jobs/truth/hold exists (run inside an attached, hidden wsl.exe session).
touch ~/regicide-jobs/truth/hold
echo "TR keepalive start pid $$ $(date -Is)" >> ~/regicide-jobs/truth/status
while [ -e ~/regicide-jobs/truth/hold ]; do sleep 60; done
echo "TR keepalive end $(date -Is)" >> ~/regicide-jobs/truth/status
