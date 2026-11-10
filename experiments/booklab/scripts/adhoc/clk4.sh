PS=/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe
w() { $PS -NoProfile -Command '[System.Diagnostics.Stopwatch]::GetTimestamp() / [System.Diagnostics.Stopwatch]::Frequency' | tr -d '\r'; }
m() { python3 -c 'import time;print(time.clock_gettime(time.CLOCK_MONOTONIC), time.clock_gettime(time.CLOCK_MONOTONIC_RAW))'; }
python3 -c 'import ctypes,ctypes.util
class T(ctypes.Structure): _fields_=[("modes",ctypes.c_uint),("offset",ctypes.c_long),("freq",ctypes.c_long),("maxerror",ctypes.c_long),("esterror",ctypes.c_long),("status",ctypes.c_int),("constant",ctypes.c_long),("precision",ctypes.c_long),("tolerance",ctypes.c_long),("tv",ctypes.c_long*2),("tick",ctypes.c_long),("pad",ctypes.c_long*20)]
t=T(); r=ctypes.CDLL(ctypes.util.find_library("c")).adjtimex(ctypes.byref(t)); print(f"adjtimex: freq {t.freq/65536:.1f} ppm, tick {t.tick} (10000 = nominal), status {t.status}")'
systemctl is-active systemd-timesyncd chronyd chrony 2>/dev/null | paste -sd' '; ps -eo comm | grep -E "chrony|timesync|ntp" | sort -u
read a1 r1 < <(m); b1=$(w); sleep 90; read a2 r2 < <(m); b2=$(w)
python3 -c "print(f'90 s: MONOTONIC {$a2-$a1:.4f}, MONOTONIC_RAW {$r2-$r1:.4f}, Windows {$b2-$b1:.4f}; Windows/MONOTONIC {($b2-$b1)/($a2-$a1):.5f}, Windows/RAW {($b2-$b1)/($r2-$r1):.5f}')"
