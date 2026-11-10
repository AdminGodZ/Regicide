#!/usr/bin/env python3
"""Compare two PE files (e.g. a rebuilt regicide_cb.dll with the production one) section by section.

  pe_compare.py A.dll B.dll
Prints the link timestamps and, per section of A, the number of differing bytes. Code and data sections (.text,
.rdata, .data, .pdata, .tls, .reloc) must match; the link timestamp, .buildid (PDB GUID) and the DWARF debug
sections (which name the build directory) differ between builds.
"""
import struct
import sys


def sections(d):
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    n = struct.unpack_from("<H", d, pe + 6)[0]
    opt = struct.unpack_from("<H", d, pe + 20)[0]
    st = pe + 24 + opt
    out = []
    for k in range(n):
        name = d[st + 40 * k:st + 40 * k + 8].rstrip(b"\0").decode()
        _, _, rawsz, raw = struct.unpack_from("<IIII", d, st + 40 * k + 8)
        out.append((name, raw, rawsz))
    return pe, out


def main():
    a, b = (open(p, "rb").read() for p in sys.argv[1:3])
    pa, sa = sections(a)
    pb, sb = sections(b)
    print(f"sizes {len(a)} / {len(b)}; link timestamps {struct.unpack_from('<I', a, pa + 8)[0]} / "
          f"{struct.unpack_from('<I', b, pb + 8)[0]}")
    if [(n, s) for n, _, s in sa] != [(n, s) for n, _, s in sb]:
        print("section layout differs")
        return 1
    bad = 0
    for name, raw, sz in sa:
        diff = sum(1 for i in range(raw, raw + sz) if a[i] != b[i])
        code = name in (".text", ".rdata", ".data", ".pdata", ".tls", ".reloc")
        bad += diff if code else 0
        print(f"{name:10} {sz:9} bytes  {diff:6} differ{'  <- code/data' if code and diff else ''}")
    print("code and data identical" if bad == 0 else "CODE OR DATA DIFFER")
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
