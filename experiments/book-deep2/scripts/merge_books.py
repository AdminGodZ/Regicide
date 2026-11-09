#!/usr/bin/env python3
# merge_books.py BASE.bin EXT.bin OUT.bin: union of two RGBK opening books. For a (position key, move) present in both,
# the entry with the greater search depth is kept (EXT on a tie), i.e. the deeper analysis wins, as Book::finalize does.
# Avoid entries of both inputs are kept (deduplicated). Entry layout (src/book.hpp): u64 key, u8 from, u8 to, u8 disc,
# u8 depth, i16 score, u16 count = 16 bytes, little endian. Header: "RGBK", u32 version (1|2), u32 count[, u32 avoid].
import struct, sys
ENT = struct.Struct('<QBBBBhH')
def load(p):
    b = open(p, 'rb').read()
    assert b[:4] == b'RGBK', p
    ver, cnt = struct.unpack_from('<II', b, 4)
    av = struct.unpack_from('<I', b, 12)[0] if ver == 2 else 0
    h = 16 if ver == 2 else 12
    es = [ENT.unpack_from(b, h + i * 16) for i in range(cnt)]
    avs = [ENT.unpack_from(b, h + (cnt + i) * 16) for i in range(av)]
    return es, avs
base, base_av = load(sys.argv[1]); ext, ext_av = load(sys.argv[2])
best = {}
for src, es in (('base', base), ('ext', ext)):
    for e in es:
        k = e[:4]
        if k not in best or e[4] > best[k][1][4] or (e[4] == best[k][1][4] and src == 'ext'):
            best[k] = (src, e)
ents = sorted((e for _, e in best.values()), key=lambda e: e[:4])
avoid = sorted({e[:4]: e for e in base_av + ext_av}.values(), key=lambda e: e[:4])
keys_b, keys_e = {e[0] for e in base}, {e[0] for e in ext}
from_ext = sum(1 for s, _ in best.values() if s == 'ext')
with open(sys.argv[3], 'wb') as f:
    f.write(b'RGBK'); f.write(struct.pack('<II', 2 if avoid else 1, len(ents)))
    if avoid: f.write(struct.pack('<I', len(avoid)))
    for e in ents + avoid: f.write(ENT.pack(*e))
print(f'base: {len(base)} entries / {len(keys_b)} positions; ext: {len(ext)} entries / {len(keys_e)} positions; '
      f'overlapping positions {len(keys_b & keys_e)}; merged: {len(ents)} entries / {len({e[0] for e in ents})} positions '
      f'({from_ext} entries from ext), avoid {len(avoid)}')
