#!/usr/bin/env bash
# train_v2.sh NAME [train.py arch/size options...]: one NNUE training run in WSL on the RTX 5080, then the export
# check and a bench. Run from the root of a t3/nnue-v2 (or later) checkout that is built in build/.
#
#   experiments/nnue_v2/scripts/train_v2.sh v2a --arch v2 --ft 512 --l1 32 --l2 32 --buckets 8
#   experiments/nnue_v2/scripts/train_v2.sh v2b --arch v2 --ft 1024 --l1 16 --l2 32 --buckets 8
#
# Environment: PY (default ~/regicide-nnue/.venv/bin/python), DATA_DIR (default ~/regicide-data/train),
# WORK (default ~/regicide-nnue-v2), EPOCHS (40), THREADS (1: leaves the CPU to matches/datagen; 0 = torch default).
set -euo pipefail
NAME=$1; shift
PY=${PY:-~/regicide-nnue/.venv/bin/python}
DATA_DIR=${DATA_DIR:-~/regicide-data/train}
WORK=${WORK:-~/regicide-nnue-v2}
EPOCHS=${EPOCHS:-40}
THREADS=${THREADS:-1}
mkdir -p "$WORK/runs"
# torch.compile fails to find nvcc when a Windows directory in PATH is unreadable (experiments/nnue1).
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd: -)
[ "$THREADS" != 0 ] && export OMP_NUM_THREADS=$THREADS

# Every finished data file; snapshots of files that were still being written (*partial*) duplicate their prefix.
DATA=()
for f in "$DATA_DIR"/*.bin; do
    case $f in *partial*) continue ;; esac
    DATA+=(--data "$f")
done
printf '%s\n' "${DATA[@]}" | paste -sd' ' - > "$WORK/runs/$NAME.data_args.txt"

THREAD_ARGS=()
[ "$THREADS" != 0 ] && THREAD_ARGS=(--threads "$THREADS" --loader-threads 1)
"$PY" tools/nnue/train.py "${DATA[@]}" --out "$WORK/runs/$NAME" "$@" \
    --epochs "$EPOCHS" --epoch-size 100000000 --batch 16384 \
    --lr 1e-3 --lr-final 1e-5 --wd 0.01 --lambda 0.7 --qat-epochs 2 --compile ${THREAD_ARGS[@]+"${THREAD_ARGS[@]}"} \
    2>&1 | tee "$WORK/runs/$NAME.log"

# Exact export check on 100,000 positions of the last data file, then the net in the engine.
"$PY" tools/nnue/check_export.py --engine build/regicide --data "${DATA[${#DATA[@]} - 1]}" \
    --checkpoint "$WORK/runs/$NAME/last.pt" --net "$WORK/$NAME.nnue" --n 100000 | tee "$WORK/runs/$NAME.check.txt"
printf 'set evalfile %s\nbench\nquit\n' "$WORK/$NAME.nnue" | build/regicide 2>&1 | grep -E "^(eval|bench|cannot)" \
    | tee "$WORK/runs/$NAME.bench.txt"
