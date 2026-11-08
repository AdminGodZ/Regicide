#!/usr/bin/env bash
# NNUE opening book from master cc9f35f (built-in n4c): bookgen --nodes 1000000 --expansions EXP --threads 10, nice 5.
# Resume after an interruption: RESUME=1 EXP=<remaining> book_n4c.sh (loads the saved book).
JB=~/regicide-jobs/book; OUT=~/regicide-data/book/book_n4c.bin; B=$JB/bin/bookgen-cc9f35f
st() { echo "$* $(date -Is)" | tee -a $JB/status >> ~/regicide-jobs/r4/status; }
mkdir -p ~/regicide-data/book
EXP=${EXP:-30000}; RA=()
if [ -n "${RESUME:-}" ]; then cp $OUT $OUT.resume-$(date +%H%M%S); RA=(--resume $OUT); LOG=$JB/book_n4c.resume-$(date +%H%M%S).log
else [ -e $OUT ] && { st "BOOK refusing: $OUT exists"; exit 1; }; LOG=$JB/book_n4c.log; fi
st "BOOK start: bookgen-cc9f35f --nodes 1000000 --expansions $EXP --threads 10 ${RA[*]} (nice 5)"
nice -n 5 $B --nodes 1000000 --expansions $EXP --threads 10 "${RA[@]}" --out $OUT > $LOG 2>&1
st "BOOK end rc=$? $(tail -n 1 $LOG)"
