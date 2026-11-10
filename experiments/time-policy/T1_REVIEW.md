# Review of time-policy test T1

Source: origin/master e670910, experiments/time-policy/README.md. Analysis only; no matches run here.

H3 is a suitable primary panel for this policy comparison. Its 156 openings were selected by seed before either
time policy was tested, both arms get the same openings and both colours, and its off-book searches give the
policy opportunities that the pinned production lines may remove. The inference is about H3 at this time control.
An improvement over tm_soft60 on H3 does not establish a positive score against Cake or an improvement on acf156.
Using the same corrected PR49 host/runner, n6c, frozen r11, one thread and hash128 for both arms is appropriate.

Clarify before play:

1. Define '+3 points'. Three match-score points over312 games is0.9615 percentage points. Three percentage points
   is9.36 match-score points; actual W/D/L scores move in half-point steps. The two thresholds differ by a factor
   of3.12. Freeze the chosen unit in the result header as well as the README.
2. The alternative acceptance path, a first-run gain above that threshold followed by any positive second run,
   is an operational adoption rule, not a95%-confidence procedure. Under a zero-effect model, a new run's sign
   can be positive about half the time. Selection on the first run plus a sign repeat does not supply a95% lower
   bound. For illustration, if the score-difference SE were3 match points and the threshold3 match points, a
   Normal independent-run model accepts about9.2% of null policies through the union of the two gates, rather
   than2.5% through the positive lower-endpoint gate alone. This is an illustration, not an estimate of T1's SE.
   The real games reuse openings and opponent behaviour, so even independence of the two estimates is limited.
3. If two runs are needed, average both repetitions inside each opening before resampling156 opening units.
   Report the second run separately and the combined opening-level interval. Do not call312 repetitions of
   openings independent units. A chosen two-stage gate needs calibration if a formal error rate is claimed.
4. H3 can show a policy gain; the all-draw production deck can still show a regression. Before deploying100 or
   starting mining under it, play a fresh acf156 Cake regression run with the frozen deployment book. A new loss
   needs explanation. A zero-loss run remains a screen, not a proof of safety.
5. Log the actual engine/host/runner/net/book hashes and options, keep controls fresh, alternate arm order across
   parts, and record policy opportunities as well as wall/charged time and bank. State whether 'bank before'
   includes this move's0.5s credit. Pinning and forced/DB returns can make a policy comparison have few searches.

The cheapest adjustment is to keep the predeclared pilot/adoption rule but label its second path provisional,
publish the combined interval, and reserve an untouched confirmation if the owner needs a95%-level gain claim.
Keeping60 when the result is unresolved is consistent with the stated objective. 'No measurable effect' must
retain its interval and sample size; it does not establish equivalence or no possible effect.

The README's median bank and time-use observations are historical wall-accounting measurements. The soft limit
explains a mechanism for saving time, but those records do not isolate how much of every engine's wall interval
was thinking, nor prove that the saved reserve caused losses. T1 is the test of that hypothesis.
