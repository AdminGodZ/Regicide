#!/usr/bin/env bash
# Rebuild every opponent engine into build/opponents/<engine>/.
set -euo pipefail
cd "$(dirname "$0")"
for s in build_simplech.sh build_cake.sh build_guinn.sh; do
  bash "./$s"
done
echo "opponents built in $(cd ../.. && pwd)/build/opponents"
