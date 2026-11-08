#!/usr/bin/env bash
# Checks t3/nnue-speed on an x86 box (AVX2 / AVX-VNNI) against the commit it started from, then compares nps.
#
#   experiments/nnue_speed/scripts/verify_x86.sh [data.bin]
#
# Run from the root of a t3/nnue-speed checkout on an otherwise idle machine (pause datagen). data.bin is any
# DataRecord file (for example a validation slice); without it a small one is generated. Environment: BASE_REV
# (default 821c89e), RUNS (bench rounds, default 6), J (build jobs, default 8).
# Builds go to build-nsv-*/, the baseline worktrees to ../regicide-nsv-base*, and logs to
# experiments/nnue_speed/logs/x86_<date>/.
set -euo pipefail
ROOT=$(pwd)
BASE_REV=${BASE_REV:-821c89e}
RUNS=${RUNS:-6}
J=${J:-8}
LOG=$ROOT/experiments/nnue_speed/logs/x86_$(date +%Y%m%d_%H%M)
mkdir -p "$LOG"
exec > >(tee "$LOG/verify.txt") 2>&1

NN2B=$ROOT/experiments/nnue2/nn2b.nnue
NN3C=$ROOT/experiments/nnue3/nn3c.nnue
NN3D=$ROOT/experiments/nnue3/nn3d.nnue

build() {  # build SRC DIR TARGET [cmake args...]
    local src=$1 dir=$2 target=$3
    shift 3
    cmake -S "$src" -B "$dir" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ -DREGICIDE_TOOLS=OFF \
        "$@" > "$dir.log" 2>&1
    cmake --build "$dir" -j "$J" ${target:+--target $target} >> "$dir.log" 2>&1 || { tail -n 30 "$dir.log"; exit 1; }
}

echo "== $(git log --oneline -1), baseline $BASE_REV"
echo "== $(clang++ --version | head -n 1)"
echo "== -march=native: $(echo | clang++ -march=native -dM -E -x c++ - | grep -oE '__(AVX2|AVXVNNI|AVX512VNNI|AVX512VL)__' | sort -u | tr '\n' ' ')"

# New code: the three architectures with -march=native (AVX-VNNI on the 285K), and the AVX2 path (maddubs).
build . build-nsv-512 ""
build . build-nsv-1024 regicide -DREGICIDE_NNUE_FT=1024
build . build-nsv-l164 regicide -DREGICIDE_NNUE_L1=64
build . build-nsv-avx2 "" -DREGICIDE_NATIVE=OFF -DCMAKE_CXX_FLAGS=-march=haswell
echo "== vpdpbusd in the native build: $(objdump -d build-nsv-512/regicide | grep -c vpdpbusd || true) (must be > 0)"
echo "== vpdpbusd in the haswell build: $(objdump -d build-nsv-avx2/regicide | grep -c vpdpbusd || true) (must be 0)"

# Baseline: the same commit with the width changed by sed, as in experiments/nnue3.
for v in 512 1024 l164; do
    W=$ROOT/../regicide-nsv-base-$v
    [ -d "$W" ] || git worktree add -q --detach "$W" "$BASE_REV"
    case $v in
        1024) sed -i 's/^constexpr int FT = 512;/constexpr int FT = 1024;/' "$W/src/nnue.hpp" ;;
        l164) sed -i 's/^constexpr int L1 = 32;/constexpr int L1 = 64;/' "$W/src/nnue.hpp" ;;
    esac
    build "$W" "$W/build" regicide
done

echo "== ctest, native (includes nnue_ft1024 and nnue_l1_64: the VNNI kernels against the naive reference)"
ctest --test-dir build-nsv-512 2>&1 | tail -n 3
echo "== ctest, -march=haswell (AVX2 maddubs path)"
ctest --test-dir build-nsv-avx2 2>&1 | tail -n 3

DATA=${1:-}
if [ -z "$DATA" ]; then
    DATA=$LOG/data.bin
    build-nsv-512/datagen --games 300 --nodes 3000 --threads 4 --seed 5 --out "$DATA" | tail -n 1
fi

declare -A NEW=([512]=build-nsv-512/regicide [1024]=build-nsv-1024/regicide [l164]=build-nsv-l164/regicide)
declare -A NET=([512]=$NN2B [1024]=$NN3C [l164]=$NN3D)
declare -A NODES=([512]=6873528 [1024]=6167330 [l164]=5185919)
base() { echo "$ROOT/../regicide-nsv-base-$1/build/regicide"; }
bench() {  # bench BINARY NET -> "nodes nps"
    printf 'set evalfile %s\nbench\nquit\n' "$2" | "$1" 2>/dev/null | awk '/^bench:/ {print $2, $6}'
}

echo "== evaldata: new vs baseline on 100000 records (must be identical)"
for v in 512 1024 l164; do
    "${NEW[$v]}" evaldata "${NET[$v]}" "$DATA" 100000 > "$LOG/eval_new_$v.txt" 2>/dev/null
    "$(base $v)" evaldata "${NET[$v]}" "$DATA" 100000 > "$LOG/eval_base_$v.txt" 2>/dev/null
    if cmp -s "$LOG/eval_new_$v.txt" "$LOG/eval_base_$v.txt"; then r=identical; else r=DIFFERENT; fi
    echo "$v: $(wc -l < "$LOG/eval_new_$v.txt") evals, $r"
done
echo "== built-in bench (must be ${NODES[512]}): $(build-nsv-512/regicide bench | tail -n 1)"

echo "== bench nodes and nps, $RUNS alternating rounds"
for i in $(seq "$RUNS"); do
    for v in 512 1024 l164; do
        echo "base_$v $(bench "$(base $v)" "${NET[$v]}")"
        echo "new_$v $(bench "${NEW[$v]}" "${NET[$v]}")"
    done
done > "$LOG/bench_runs.txt"
for v in 512 1024 l164; do
    for side in base new; do
        runs=$(grep "^${side}_$v " "$LOG/bench_runs.txt")
        nodes=$(echo "$runs" | awk '{print $2}' | sort -u | tr '\n' ' ')
        nps=$(echo "$runs" | awk '{print $3}' | sort -n | tr '\n' ' ')
        median=$(echo "$nps" | tr ' ' '\n' | grep . | awk '{a[NR] = $1} END {print NR % 2 ? a[(NR + 1) / 2] : int((a[NR / 2] + a[NR / 2 + 1]) / 2)}')
        printf '%-9s nodes %s(expect %s)  median nps %s  sorted runs %s\n' "${side}_$v" "$nodes" "${NODES[$v]}" "$median" "$nps"
    done
done | tee "$LOG/bench_summary.txt"

echo "== stage timings (new code)"
for v in 512 1024 l164; do "${NEW[$v]}" nnuebench "${NET[$v]}" "$DATA" 1024 2>/dev/null; done
echo "== logs in $LOG"
