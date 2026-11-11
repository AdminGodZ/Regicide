# Fixture (review 2026-11-11 residual): from B:W21,K22,23,...,32:B1,2,3,4,K5,9 the lost game 9-13 22-18 5-9 and the
# drawn game 9-13 22-18 1-6; the base book avoids only 5-9 (after 9-13 22-18, inside a king-move run).
L=~/regicide-jobs/booklab; W=~/regicide-booklab; J=/mnt/c/regicide-match/jobs/booklab; T=$L/fx_avoid; rm -rf $T; mkdir -p $T/run
S=$J/src/tools/booklab; FEN0="B:W21,K22,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,K5,9"
BASE=$L/book_master.bin; [ -f $BASE ] || BASE=$(ls $L/book_r10k5.bin)
F2=$(printf "0\t$FEN0\tB\t0\t9-13 22-18 5-9\n" | $W/build-linux/booklab_pinwalk $BASE | awk -F'\t' '$2==2 {print $3}')
echo "position of 5-9: $F2"
echo "$F2 | 5-9 | avoid | fixture: the lost game's move" > $T/dec.txt
$W/build-linux/booklab_apply --book $BASE --decisions $T/dec.txt --out $T/book.bin > $T/apply.log 2>&1 || { cat $T/apply.log; exit 1; }
g() { printf '[Event "fx"]\n[Black "Regicide 0.2"]\n[White "Cake 1.89f"]\n[Round "%s"]\n[Opening "fixture"]\n[FEN "%s"]\n[SetUp "1"]\n[Result "%s"]\n\n1. 9-13 22-18 2. %s %s\n\n' "$1" "$FEN0" "$2" "$3" "$2"; }
{ g 1.1 "0-1" "5-9"; g 2.1 "1/2-1/2" "1-6"; } > $T/run/games.pdn
cd $T && python3 $S/pins.py --runs $T/run --book $T/book.bin --pinwalk $W/build-linux/booklab_pinwalk --out pins.txt --report pins.md --egdb ~/regicide-data/egdb > /dev/null
echo "pins:"; grep -v "^#" pins.txt
printf "0\t$FEN0\tB\t0\t9-13 22-18 5-9\n" | $W/build-linux/booklab_pinwalk $T/book.bin
