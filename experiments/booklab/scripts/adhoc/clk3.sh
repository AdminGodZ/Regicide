PS=/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe
w() { $PS -NoProfile -Command '[System.Diagnostics.Stopwatch]::GetTimestamp() / [System.Diagnostics.Stopwatch]::Frequency' | tr -d '\r'; }
m() { python3 -c 'import time;print(time.monotonic())'; }
echo "clocksource: $(cat /sys/devices/system/clocksource/clocksource0/current_clocksource); $(dmesg 2>/dev/null | grep -o 'tsc: Detected [0-9.]* MHz processor' | head -1)"
for n in 1 2; do
  a1=$(m); b1=$(w); a1b=$(m); sleep 90; a2=$(m); b2=$(w); a2b=$(m)
  python3 -c "
a1=($a1+$a1b)/2; a2=($a2+$a2b)/2
print(f'measurement $n: WSL monotonic {a2-a1:.4f} s, Windows QPC {$b2-$b1:.4f} s, ratio Windows/WSL {($b2-$b1)/(a2-a1):.5f} (bracket {$a1b-$a1:.2f}/{$a2b-$a2:.2f} s)')"
done
