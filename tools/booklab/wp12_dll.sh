#!/usr/bin/env bash
# WP12: candidate engine = the r11 DLL with its embedded book replaced by a one-key override of r11 (same size, so
# nothing else in the binary moves). Nothing is compiled; the engine code is byte-identical to bl-456e9b2-r11.
#   wp12_dll.sh NAME FEN MOVE   ->  C:\regicide-match\engines\regicide\bl-w12-NAME\regicide_cb.dll (+ egdb junction)
set -euo pipefail
NAME=$1; FEN=$2; MOVE=$3
L=~/regicide-jobs/booklab; BASE=/mnt/c/regicide-match/engines/regicide/bl-456e9b2-r11
D=/mnt/c/regicide-match/engines/regicide/bl-w12-$NAME; T=$L/wp12/books; mkdir -p $T
$L/bin/blk-override $L/book_r11.bin $T/$NAME.bin "$FEN" "$MOVE" > $T/$NAME.txt
mkdir -p $D
python3 - $BASE/regicide_cb.dll $L/book_r11.bin $T/$NAME.bin $D/regicide_cb.dll <<'PY'
import sys
dll, old, new = (open(p, 'rb').read() for p in sys.argv[1:4])
assert len(old) == len(new) and dll.count(old) == 1, 'base book not found exactly once or size changed'
i = dll.find(old)
out = dll[:i] + new + dll[i + len(old):]
assert len(out) == len(dll) and sum(a != b for a, b in zip(out, dll)) == sum(a != b for a, b in zip(old, new))
open(sys.argv[4], 'wb').write(out)
PY
if [ ! -e $D/egdb ]; then
  /mnt/c/Windows/System32/cmd.exe /c mklink /J "C:\\regicide-match\\engines\\regicide\\bl-w12-$NAME\\egdb" \
    "C:\\regicide-match\\engines\\regicide\\6bb8c6c-deep2b-egdb8\\egdb" > /dev/null
fi
{ echo "booklab WP12 candidate $NAME: bl-456e9b2-r11 regicide_cb.dll with the embedded book replaced by"
  echo "  $(cat $T/$NAME.txt) at $FEN"
  echo "book sha256 $(sha256sum $T/$NAME.bin | cut -c1-16); dll sha256 $(sha256sum $D/regicide_cb.dll | cut -c1-16)"; } > $D/BUILD.txt
cat $D/BUILD.txt
