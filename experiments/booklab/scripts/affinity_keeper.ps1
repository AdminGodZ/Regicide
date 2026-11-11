# Affinity keeper for the three concurrency-1 match-order runs (lead 2026-11-11, item 3).
# Each run uses its own byte-identical copy of the 1c6e997 host: C:\regicide-match\bin\1c6e997-c1-<opp>\cbhost.exe.
# Every 3 s: cbhost.exe processes of the copy for kingsrow -> core 21, cake -> core 22, guinn -> core 23 (the two
# engines of a run alternate, so one core each); every other cbhost.exe (the 8-worker runs) -> cores 0-20.
# The WSL VM (other agents' jobs) is not touched. Stops when the stop file exists or after 8 hours.
# Log: C:\regicide-match\jobs\booklab\affinity_keeper.log (one line per change, counts every 10 minutes).
$log = "C:\regicide-match\jobs\booklab\affinity_keeper.log"
$stop = "C:\regicide-match\jobs\booklab\affinity_keeper.stop"
# host folders <commit>-c1-<opp> (1c6e997 for the first three runs, 8de39d0 for the --cb-claims runs)
# "-c1-tb": the #6 repair test bed (lead 12:35), core 20
$cores = @{ "-c1-kingsrow" = 21; "-c1-cake" = 22; "-c1-guinn" = 23; "-c1-tb" = 20 }
$rest = [IntPtr]0xFFFFF   # cores 0-19
$t0 = Get-Date; $last = Get-Date
"start $(Get-Date -Format s)" | Out-File -Append -Encoding utf8 $log
while (-not (Test-Path $stop) -and ((Get-Date) - $t0).TotalHours -lt 8) {
  $n = @{}
  foreach ($p in Get-Process cbhost -ErrorAction SilentlyContinue) {
    try {
      $path = $p.Path; $want = $rest; $key = "rest"
      foreach ($k in $cores.Keys) { if ($path -like "*$k\*") { $want = [IntPtr](1 -shl $cores[$k]); $key = $k } }
      $n[$key] = 1 + [int]$n[$key]
      if ($p.ProcessorAffinity -ne $want) {
        $p.ProcessorAffinity = $want
        "$(Get-Date -Format s) pid $($p.Id) $key -> 0x$('{0:X}' -f [int64]$want)" | Out-File -Append -Encoding utf8 $log
      }
    } catch { }
  }
  if (((Get-Date) - $last).TotalMinutes -ge 10) {
    "$(Get-Date -Format s) processes: $(($n.GetEnumerator() | ForEach-Object { "$($_.Key)=$($_.Value)" }) -join ' ')" | Out-File -Append -Encoding utf8 $log
    $last = Get-Date
  }
  Start-Sleep -Seconds 3
}
"end $(Get-Date -Format s)" | Out-File -Append -Encoding utf8 $log
