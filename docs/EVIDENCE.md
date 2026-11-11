# Regicide evidence sheet

Snapshot: 2026-11-10, review round 14. This page records observations and the inference they support. W/D/L is
from Regicide's view, or the named candidate net's view. A win scores 1, a draw 0.5. pp means percentage points.

Verdicts apply to the wording of each row: **established** = reproduced fact in the named records;
**supported** = inference supported within its stated conditions; **provisional** = selected, small, or
report-only evidence with an unresolved limitation; **not shown** = the claimed extension is absent or not
established by those data. Zero losses in a sample never means a zero future loss rate.

Branches are pinned so later runs do not silently change a row. M=origin/master, B=origin/t3/booklab,
T=origin/t3/truth-data, L=origin/t3/search-lab. Links below point to these commits:

- M: `33156cf86918ba12680b6305ae039250b2f83786`
- B: `2d85e83eb95d37dbb070c5d51825cc5303c13bef`
- T: `74e7755470f0fc05cf34d22df3db46e090f45308`
- L: `5f450ef968766cca164255d3b3639085c56c99ec`

Shared conditions used below:

- **F**: Windows production/opponent experiments, acf156 unless another panel is named, 0.5 s/move CheckerBoard
  banking, one search thread and hash 128 for Regicide, opponent books on, Regicide 2-8 database and opponents'
  KingsRow 2-8 WLD database. Cake 1.89f, KingsRow 1.20, GuiNN 2.06 avx2. Historical runner wall time charged to banks,
  before PR #49. KingsRow mode 1 except the explicit mode 3 runs; others mode 3. Not fixed 0.5-second searches.
- **N**: net self-play, both sides engine fcf6acb/rtruth.so, book off, thread 1/hash 128,2-8 DB,805 four-ply
  openings with both colours,0.1s banked CheckerBoard time unless stated. Historical clock, not PR #49 accounting.
- **L**: laptop net self-play, both sides engine 456e9b2, book off, thread 1/hash 128,2-7 DB, same 805 openings,
  0.5s banked CheckerBoard time, six games each on its own physical P-core. Historical runner source, before
  PR #49 accounting. Installed binary hashes are not independently recoverable from game rows.

Elo intervals from tools/match use population pair variance and transformed score endpoints. Pooled n8bn0.5s
and laptop intervals use sample pair variance and a delta-method Elo interval. Pentanomial order is
[LL,LD,DD/WL,WD,WW]. They are nominal intervals, not selection-adjusted. Holdout differences use 50,000
percentile bootstrap resamples of openings, retaining both colours and both arms, seed 2026111013. These
intervals describe variation across the tested openings; they do not estimate future machine/load variation.
Raw game results were recounted; database annotations were not independently reprobed with 8-piece tables.

| Claim | Numbers | Source and verification | Conditions and limit | Verdict |
|---|---|---|---|---|
| The opponent census favored Cake under its tested settings. | Cake/KingsRow mode 1:13/299/0; Cake/GuiNN:16/294/2; KingsRow mode 1/GuiNN:2/303/7,312 games each. | [B: c-cake-kingsrow-*, c-cake-guinn-*, c-kingsrow-guinn-* CSVs][B-1]; recomputed. | F; KingsRow 1.20 mode 1 in both census pairings. One repetition on acf156; no ranking against KingsRow mode 3 or across openings/versions. | supported |
| The old engine failed to convert database wins in recorded games. | 37/389 converted; 352 missed games, 339 first-won boards. The 362-game count is superseded. | [B: wp8/dbscan_games.tsv.gz][B-2]; recomputed scanner rows. | Mixed historical builds and selected production-deck games; database annotations recounted, not reprobed. Final positions after a rule draw excluded. | established |
| Conversion search converts more generated small-table wins at the tested budgets. | 3/4 pieces, cap 0: old 155/400 and 218/400; new 400/400 each at 20k nodes/move. | [M: before/after harness logs][M-3]; log summaries recomputed. | Same generated root family; chosen/tuned on this family, not a random estimate of all endings. The reference database policy is not distance-optimal. | supported |
| Conversion is budget-dependent on real tables. | 6 pieces, cap 4: old 345/400, new 400/400 at 20k. Real 7-piece new: 188/200 at 5k, 194/200 at 20k, 199/200 at 100k. | [M: R6/R7 before/after logs][M-4]; recomputed. | Selected men-on-both-sides roots. Above six pieces the defender is conversion search; no comparable old real-7 control. Not a universal conversion guarantee. | supported |
| The conversion build won the selected missed-win regression games. | Corrected set 352/352, 339 boards; old matched subset 1/55 versus new 55/55. Later check 98/98 reused those roots. | [B: wp9 manifests][B-5] and [B: tests/wp9base-*, wp9conv1-*, wp9conv3-*][B-6]; manifests joined to CSVs. | Roots selected because earlier games missed their wins; original opponents resumed from fixed roots at 0.5s banking. Repeats/subsamples are not independent root confirmation. | established |
| The conversion build converted more wins reached in full production games. | Step 4: KingsRow mode 1 new 11/11 versus old 0/13; GuiNN 20/20 versus 5/21. W/D/L: new 11/300/1 and 20/292/0; old 0/312/0 and 6/306/0. | [B: experiments/booklab/data/s4_dbscan_games.tsv.gz][B-7] and [B: tests/s4-* CSVs][B-8]; recomputed. | F; same book r6, different engine builds and reached positions; KingsRow mode 1. Supports conversion benefit, not an isolated same-position causal rate. | supported |
| The repaired-book n6c validation had no losses against Cake. | Run a 0/312/0; run b 0/312/0. These runs used r8; 148,864 recorded position rows have identical r8/r10 probes. | [B: bl-f1-cake-*.pdn.gz][B-9] and [B: experiments/booklab/data/book_equiv_r10_positions.tsv.gz][B-10]; recomputed. | F; engine 2efe11a, n6c, r8. R10 transfer rests on recorded-line book equivalence, not 624 fresh r10 games. The production ballots supplied the repairs. | established |
| The repaired-book n6c validation scored against KingsRow in mode 1. | 12/300/0 and 11/301/0; total 23/601/0. | [B: bl-f1-kingsrow-*.pdn.gz][B-11]; raw games and CSVs recomputed. | F; KingsRow 1.20 book mode 1, two repetitions, r8 with recorded-line r10 equivalence. This is not the intended mode-3 result. | established |
| The repaired-book n6c validation scored against GuiNN. | 17/295/0 and 21/291/0; total 38/586/0. | [B: bl-f1-guinn-*.pdn.gz][B-12]; recomputed. | F; GuiNN 2.06 avx2, book mode 3, two repetitions, r8 with recorded-line r10 equivalence. GuiNN loads its book lazily. | established |
| R10/n6c lost games against KingsRow in mode 3. | 2/307/3 and 1/307/4; total 3/614/7. All seven losses as Black, five ballot/colour pairs, four lines after a transposition. | [B: experiments/booklab/k3/games/][B-13] and [B: experiments/booklab/k3/runs/][B-14]; raw games/CSV recomputed. | F except explicit KingsRow mode 3 read-back; engine 456e9b2, r10, two repetitions, legacy match-747ab51 clock. Replayed attempt after reported memory/crash failures. | established |
| The new bundle improved on H1 against the tested opponents. | New/old W/D/L: Cake 11/10/15 vs 0/21/15; KingsRow mode 1 15/3/18 vs 3/15/18; GuiNN 14/15/7 vs 2/26/8. Paired score gains +15.28, +16.67, +18.06 pp. | [B: w10-b3-*/results.csv][B-15] and [B: Block 3 PDNs][B-16]; recomputed by opening. | F; H1 all 18 barred ballots, both colours, one repetition per arm. O=fcf6acb/old book; EB=456e9b2/r10; n6c. KingsRow mode 1. Conditional on these starts. | supported |
| H2 is a 19-opening pilot, not an equivalence test. | New/old: Cake 4/29/5 vs 2/32/4; KingsRow mode 1 7/27/4 vs 2/32/4; GuiNN 5/27/6 vs 0/32/6. Gains [95%]: +1.32 [-2.63,5.26], +6.58 [2.63,11.84], +6.58 [0,13.16] pp. | [B: w10-b3-*-H2/results.csv][B-17]; recomputed. | Same arms/conditions as H1, 19 certified-clean four-ply starts, one repetition. Cake and GuiNN do not exclude zero. | provisional |
| The new bundle improved against Cake on H3. | New 57/197/58; old 14/240/58. Paired score gain +6.89 pp [5.13,8.65]. New absolute score 49.84%. | [B: w10-b3-*-cake-H3_*/results.csv][B-18]; recomputed opening bootstrap. | F; 156 seeded unresolved four-ply starts, both colours, one repetition. This is improvement over O, not a positive score against Cake or a production-deck win claim. | supported |
| The new bundle improved against KingsRow on H3 in mode 1. | New 58/199/55; old 14/245/53. Paired score gain +6.73 pp [4.97,8.49]. | [B: w10-b3-*-kingsrow-H3_*/results.csv][B-19]; recomputed opening bootstrap. | F; H3, one repetition. KingsRow mode 1 only; no H3 mode-3 comparison is committed. | supported |
| The new bundle improved against GuiNN on H3. | New 65/197/50; old 17/243/52. Paired score gain +8.01 pp [5.93,10.10]. | [B: w10-b3-*-guinn-H3_*/results.csv][B-20]; recomputed opening bootstrap. | F; H3, one repetition. Interval resamples opening effects, not future run/load variation or other opponent versions. | supported |
| Direct use of r10 repairs does not explain Block 3 gains. | Zero changed-key contacts on all 128,318 Regicide calls, O and EB. H3 annotated conversions EB/O: Cake 53/53 vs 14/57; KingsRow mode 1 58/58 vs 13/57; GuiNN 64/64 vs 16/63. | [B: experiments/booklab/wp10/games/][B-21] and [B: experiments/booklab/wp10/games.tsv][B-22]; prior independent key replay revalidated against unchanged raw files; annotation counts recomputed. | F; H1-H3 only. Opponent-turn contacts are not exposure. Different reached endgames, no fresh eight-piece oracle run and no isolated ablation of every engine change. | supported |
| Block 4 establishes a book increment on touched openings. | Only E/Cake/H2x first 78 openings complete: rerun 13/122/21, superseding14/120/22. No paired EB arm or tier estimate yet. | [B: experiments/booklab/tests/w10-b4-E-cake-H2x_0/results.csv][B-23]; recomputed. | Partial availability; full H2x has 156 openings with two exposure tiers. No completed E/EB comparison at this snapshot. | not shown |
| The old training corpus contains result/database disagreements. | 405,095/2,666,528 games reaching <=8 pieces disagree, 15.19%; 13.99% of all 2,894,895 games. 58.54% of 294,484,169 records have <=8 pieces. | [T: experiments/truth-data/logs/census.txt][T-24]; histograms, matrices and denominators recomputed. | Listed legacy training files excluding gen4b. Counts describe the census, not unique positions; no causal claim that every disagreement changes strength. | established |
| The corrected pilot proves many stored positions and disagrees with some game labels. | 200,000 records; 59,990 proven at 10k, 75,246 at 50k, 87,370 at 200k. 15,227/87,370 game labels disagree, 17.43%. | [T: experiments/truth-data/data/pilot2_label_rule.csv.gz][T-25]; per-record arithmetic recomputed. | Sampled 9-14-piece records; fresh-hash/no-history search with 8-piece DB and label_of. Conditional on proofs at the stated budget. Supersedes the first pilot. | established |
| n8bn had a positive self-play estimate at 0.1s. | 276/1079/255, penta [0,36,712,57,0], +4.5 Elo [0.5,8.6]. Seed-2 recipe replicate +3.0 [-0.9,7.0]. | [T: experiments/truth-data/matches/n8bn-vs-n6c/results.csv][T-26] and [T: experiments/truth-data/matches/n8bn2-vs-n6c/results.csv][T-27]; recomputed. | N; best of five initial recipes, eleven later, mostly one training seed. The selected 0.1s interval is nominal and does not survive that selection allowance. | provisional |
| n8bn had a positive estimate in the 0.5s confirmation. | 267/1097/246, penta [0,13,758,34,0], +4.5 Elo [1.6,7.4]. | [T: n8bn-vs-n6c-0.5-{a,b,c,d}/results.csv][T-28]; pooled pair statistics recomputed. | N except 0.5s; four chunks, same 805 openings, chosen after the 0.1s screen. New games/time control, not a new opening sample. | supported |
| The K/lambda/phase sweeps establish n8bn as the best recipe. | K130 +3.2 [-0.6,7.1]; lambda .6 -0.2 [-4.1,3.7], .7 +3.0 [-0.8,6.9]. Phase nets vs n8bn +0.4 [-3.6,4.5], -0.4 [-4.2,3.3]. | [T: sweep and phase-net result CSVs][T-29]; recomputed. | N; no demonstrated improvement over n8bn. This does not prove recipe optimality or equivalence at 1-2 Elo; indirect comparisons share n6c/openings. | not shown |
| n8c improves playing strength over n8bn. | n8c/n6c at 0.1s: +0.4 [-3.3,4.2]. Direct n8c/n8bn at 0.5s: 262/1078/270, penta [0,41,731,33,0], -1.7 [-5.4,1.9]. | [T: experiments/truth-data/matches/n8c-vs-n6c/results.csv][T-30] and [L: experiments/search-lab/netm/matches/M5_n8c-vs-n8bn/][L-31]; recomputed. | N for 0.1s; L for direct 0.5s. Changing labels/engine/DB between experiments prevents a pooled causal estimate. | not shown |
| n8bn is unsuitable as a general built-in net without a database; n8q addresses the measured ending issue. | Reported three-kings/one fixture: n6c +517, n8bn 0, n8q +519; 400 sampled endings also reported. n8bn search test 26/27, n8q 27/27. | [T: experiments/truth-data/logs/sanity_n8q.md][T-32] and [T: T4 test report][T-33]; both fixture evaluations recomputed;400-ending aggregates and ctest counts taken from reports. | Static examples and stated test build; not all endings or a proof of no-database playing strength. Master still embeds n6c. | supported |
| n8q showed no detected loss from its low-piece training share in the 0.1s comparisons. | With DB vs n8bn 265/1087/258, +1.5 [-2.1,5.1]; without DB vs n6c 289/1045/276, +2.8 [-1.2,6.8]. | [T: experiments/truth-data/matches/n8q-vs-n8bn/results.csv][T-34] and [T: experiments/truth-data/matches/n8q-vs-n6c-nodb/results.csv][T-35]; recomputed. | N; DB 2-8 or none as stated. CIs permit several Elo in either direction; no noninferiority margin was proved. | provisional |
| n8q had a positive self-play estimate at 0.5s on the laptop. | 265/1096/249, penta [0,23,743,39,0], +3.45 Elo [0.13,6.78], 805 pairs. | [L: experiments/search-lab/netm/matches/M1_n8q-vs-n6c/][L-36]; all six chunks and sample-variance interval recomputed. | L; one repetition, same 805 openings, DB 2-7, book off. Nominal interval after net development; does not predict opponent or r10 compatibility. | provisional |
| The wide nets showed a playing gain at 0.1s. | Versus n8bn: w1064 -0.9 [-4.6,2.9]; w2032 -1.5 [-5.6,2.6]; w2064 -6.3 [-10.2,-2.3], 805 pairs each. No completed 0.5s wide-net match yet. | [T: w1064/w2032/w2064-vs-n8bn CSVs][T-37]; recomputed. | 0.1s, same wide-capable build c0dde2e, book off, 2-8 DB, same openings. First two unresolved; w2064 negative in this test. | not shown |
| The wider nets reduced measured bench throughput on the test machine. | Median NPS: n8bn 3.228M; w1064 2.493M, w2032 2.170M, w2064 1.826M; -22.8%, -32.8%, -43.4%. | [T: experiments/truth-data/logs/p5_speed.txt][T-38]; three logged runs per net, medians checked. | One machine/build. Bench node counts differ across nets, so these are workload-dependent throughput measurements, not an isolated forward-pass benchmark or cross-CPU prediction. | supported |
| Truth-data nets improve results against the opponents with our book off. | Conversion build, Cake n8bn/n8c/n6c: 0/307/5, 0/307/5, 0/305/7. GuiNN: 16/294/2, 17/293/2, 19/290/3. Earlier KR mode 1: n8bn 1/311/0, n6c 1/309/2. | [T: tr-*-c0dde2e and tr-*-kingsrow CSVs][T-39]; recomputed. | acf156, 0.5s legacy banking, thread 1/hash 128, our book off and opponents books on. KR earlier DLL differs from conversion-build runs. No net superiority established by these counts. | not shown |
| The zero-loss production result depends on which net plays after the book. | Block2 n8q Cake 0/307/5 versus n6c 0/624/0; KR mode 1 12/300/0; GuiNN 18/294/0. All five Cake losses unexposed to changed r10 positions. | [B: bl-f4-*.pdn.gz and bl-f1-* controls][B-40]; WDL/departures recomputed; exposure from checked key replay. | F; n8q one repetition, n6c two historical repetitions. Four distinct loss situations because #37/#75 transpose. Demonstrates compatibility failure, not a population net-Elo estimate. | established |
| The five n8q Cake losses began with our choices in the opening/middlegame. | First departure from both n6c games: plies 15/15/13/11/6 at 20/20/22/22/22 pieces. First search scores <=-50 occur at 17-22 pieces. In 69/78 EB/Cake holdout losses, the first logged search was already <=-50. | [B: Block2/Block3 raw scores and moves][B-41]; independent replay and prior time-window arithmetic, source bytes revalidated. | Own search scores and divergence are diagnostics, not a proof of the earliest losing move. More time has not been shown to repair these choices. | supported |
| Final-validation games vary between repetitions. | Identical: Cake 54/312, KR mode 1 0/312, GuiNN 13/312. First deviator us/opponent: 158/100, 12/300, 77/222. Overall median first differing ply 24/5/11; result changes 0/21/26. | [B: bl-f1-*.pdn.gz][B-42]; canonical dual-parser replay recomputed. | F, two repetitions. Cake-first deviations alone have median ply32; 32 is not the median over all differing Cake games. No opponent-independent repeatability claim. | established |
| KingsRow mode 3 makes entire games deterministic. | Only 4/312 mode 3 games identical; 308 differ, first by KR259/us49, median ply42 at 10 pieces. Six result changes. Mode1/mode 3: no identical games, median first difference ply5. | [B: experiments/booklab/k3/games/][B-43] and [B: bl-f1-kingsrow-*][B-44]; canonical replay recomputed. | Mode3 book probing is deterministic in the committed probe, but timed searches/game continuations are not. Engine builds also differ between historical validations. | not shown |
| Regicide held a larger reserve and used a smaller fraction of offered time in the recorded phase. | 20-24 pieces, non-instant multi-choice mean wall seconds R/C/K-mode 1/G: .417/.936/1.527/.753; median wall/offer .404/.729/.879/.908. Median bank at <=8: 3.28/1.45/.97/1.18s. | [M: three tables and TIME_POLICY.md][M-45]; prior independent reconstruction, all raw input bytes revalidated. | F, 1,872 games. Wall accounting before PR #49, conditional different positions/banks, KR mode 1. Excludes instant/multi-choice ambiguity by explicit >=.05s and >1-legal-move filters. | established |
| The whole final bank could have been spent in the early middlegame. | Same DB-entering Regicide games: mean bank 3.32 s at <=8 plus 15.08s gained later =18.40s at end. The later credit was unavailable earlier. | [M: experiments/time-policy/TIME_POLICY.md][M-46]; prior arithmetic revalidated. | F; database-sized boundary, not a claim that every such root is solved instantly. A few-second reserve hypothesis remains; reclaiming18s in advance is invalid. | not shown |
| Increasing tm_soft from 60 to 100 improves play. | Stable target threshold 0.3M to 0.5M; model reserve 2.92s to1.25s. No T1 result committed. | [M: experiments/time-policy/README.md][M-47] and [M: src/search.cpp][M-48]; mechanism read, no policy games here. | Planned H3 Cake comparison: r11 after validation, n6c, 312 games/arm, corrected PR #49 host/runner. Clarify +3-point units and distinguish the sign-repeat adoption gate from a 95% inference. | not shown |
| Pinned r11 has been shown to reproduce drawn games with either net. | Historical payload:27,418 pins, 28,405 forced entries, SHA256 2509043e...; it is to be superseded. No committed WP11 playing validation yet. | [B: experiments/booklab/wp11/book_r11.bin][B-49] and [B: experiments/booklab/pins.txt][B-50]; current hashes match the previously checked payload; playing claim pending. | Lead decision: repair four KR mode 3 lines in WP13, revalidate in mode 3, then rebuild pins and validate the replacement r11 before T1/mining. This is a plan, not a measured result. Five n8q losses not yet shown removed. | not shown |
| Search-lab found a confirmed search-strength gain. | tm_nodes screen +5.0 [0.4,9.5], confirmation +3.0 [-2.9,9.0] with 1.9% more time; other screens unresolved. lmr_base comparison void due shared table. | [L: experiments/search-lab/logs/matches/][L-51] and [L: experiments/search-lab/RESULTS.md][L-52]; aggregate logs recomputed. | Historical self-play, selected UHO set, eight ideas/variants. Before PR #37, --movetime screens stopped early; B6 used --target. No equivalence conclusion or current production gain. | not shown |
| Thread scaling and the UHO calibration establish general playing gains. | Reported/recomputed 50-root TTD speedups about 1.52x at 2 threads and 2.20-2.99x at 4. UHO sensitivity ratio 2.48 from two calibration contrasts. | [L: experiments/search-lab/logs/ttd/][L-53] and [L: experiments/search-lab/logs/matches/][L-54]; logged arithmetic checked. | One hybrid laptop, chosen roots/contrasts, placement variation and historical timing semantics. Timing-to-depth and sensitivity are not strength gains against opponents. | not shown |
| Regicide wins against Cake on the production deck, or mined wins are already validated. | Production n6c repaired-book validation 0 wins/624, n8q 0/312. H3 has57 wins and58 losses; H1/H3 winning starts do not supply production-deck wins. No discovery/confirmation result yet. | [B: experiments/booklab/wp10/games/][B-55] and [M: experiments/booklab/wp12/PROTOCOL.md][M-56]; records checked, mining pending. | Cake 1.89f, acf156 for production. Any future learned wins require separate reporting, controls, exposure and the adopted gates. | not shown |
| These results establish performance outside the tested conditions. | No external holdout evidence beyond H1-H3; no completed H3 KR mode 3 test; no production n6c/r10 multi-thread/other-time-control strength conclusion; no current r11/net guarantee. | [B: experiments/booklab/RESULTS.md][B-57], [T: experiments/truth-data/RESULTS.md][T-58] and [L: experiments/search-lab/RESULTS.md][L-59]; availability checked. | Net self-play at0.1/0.5s and thread timing tests exist, but do not fill those external-game gaps. No claim about other opponent versions, all openings or all endings. | not shown |

Artifact identity: master still embeds n6c,298,828 bytes, SHA256
`6b335193073ef5f076e28b98407f75da194902705e9cf35b85f43155d29efc5e`.
The truth-data branch's n8q candidate is 298,828 bytes, SHA256
`681549690f165eed94edafd958b06f2f76cf44ab5fba6654c82eab183efe9a24`;
n8bn is 298,828 bytes, SHA256 `c69ba616af5a9a18a90348568c3f317c236e870bb6e3b3053daea27fa73678a7`.
R10 is 561,632 bytes, SHA256 `cb4e2bb2aea4d6de6bc789513baa8b49ae70b06d5d37b064adbf1a6018197a1e`.
Historical r11 is 1,000,320 bytes, SHA256 `2509043e0ee4ee60a89eda256f369892b67640a6b58ec3249dd7f7f329243b29`.
The lead has ordered its replacement after WP13 repairs and KingsRow mode 3 revalidation. The replacement payload
and its playing validation are not committed at this snapshot. These are file identities, not playing verdicts.

The last row should remain until a named experiment supplies the missing evidence. Update a row's source commit,
conditions and verdict together when new results arrive. Keep selected book-learning results separate from
fresh-opening engine comparisons.

[B-1]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/tests
[B-2]: https://github.com/AdminGodZ/Regicide/blob/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/wp8/dbscan_games.tsv.gz
[M-3]: https://github.com/AdminGodZ/Regicide/tree/33156cf86918ba12680b6305ae039250b2f83786/experiments/wld-conversion/logs
[M-4]: https://github.com/AdminGodZ/Regicide/tree/33156cf86918ba12680b6305ae039250b2f83786/experiments/wld-conversion/logs
[B-5]: https://github.com/AdminGodZ/Regicide/blob/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/wp9/all362/positions.tsv
[B-6]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/tests
[B-7]: https://github.com/AdminGodZ/Regicide/blob/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/data/s4_dbscan_games.tsv.gz
[B-8]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/tests
[B-9]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/wp10/games
[B-10]: https://github.com/AdminGodZ/Regicide/blob/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/data/book_equiv_r10_positions.tsv.gz
[B-11]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/wp10/games
[B-12]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/wp10/games
[B-13]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/k3/games
[B-14]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/k3/runs
[B-15]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/tests
[B-16]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/wp10/games
[B-17]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/tests
[B-18]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/tests
[B-19]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/tests
[B-20]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/tests
[B-21]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/wp10/games
[B-22]: https://github.com/AdminGodZ/Regicide/blob/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/wp10/games.tsv
[B-23]: https://github.com/AdminGodZ/Regicide/blob/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/tests/w10-b4-E-cake-H2x_0/results.csv
[T-24]: https://github.com/AdminGodZ/Regicide/blob/74e7755470f0fc05cf34d22df3db46e090f45308/experiments/truth-data/logs/census.txt
[T-25]: https://github.com/AdminGodZ/Regicide/blob/74e7755470f0fc05cf34d22df3db46e090f45308/experiments/truth-data/data/pilot2_label_rule.csv.gz
[T-26]: https://github.com/AdminGodZ/Regicide/blob/74e7755470f0fc05cf34d22df3db46e090f45308/experiments/truth-data/matches/n8bn-vs-n6c/results.csv
[T-27]: https://github.com/AdminGodZ/Regicide/blob/74e7755470f0fc05cf34d22df3db46e090f45308/experiments/truth-data/matches/n8bn2-vs-n6c/results.csv
[T-28]: https://github.com/AdminGodZ/Regicide/tree/74e7755470f0fc05cf34d22df3db46e090f45308/experiments/truth-data/matches
[T-29]: https://github.com/AdminGodZ/Regicide/tree/74e7755470f0fc05cf34d22df3db46e090f45308/experiments/truth-data/matches
[T-30]: https://github.com/AdminGodZ/Regicide/blob/74e7755470f0fc05cf34d22df3db46e090f45308/experiments/truth-data/matches/n8c-vs-n6c/results.csv
[L-31]: https://github.com/AdminGodZ/Regicide/tree/5f450ef968766cca164255d3b3639085c56c99ec/experiments/search-lab/netm/matches/M5_n8c-vs-n8bn
[T-32]: https://github.com/AdminGodZ/Regicide/blob/74e7755470f0fc05cf34d22df3db46e090f45308/experiments/truth-data/logs/sanity_n8q.md
[T-33]: https://github.com/AdminGodZ/Regicide/blob/74e7755470f0fc05cf34d22df3db46e090f45308/experiments/truth-data/RESULTS.md
[T-34]: https://github.com/AdminGodZ/Regicide/blob/74e7755470f0fc05cf34d22df3db46e090f45308/experiments/truth-data/matches/n8q-vs-n8bn/results.csv
[T-35]: https://github.com/AdminGodZ/Regicide/blob/74e7755470f0fc05cf34d22df3db46e090f45308/experiments/truth-data/matches/n8q-vs-n6c-nodb/results.csv
[L-36]: https://github.com/AdminGodZ/Regicide/tree/5f450ef968766cca164255d3b3639085c56c99ec/experiments/search-lab/netm/matches/M1_n8q-vs-n6c
[T-37]: https://github.com/AdminGodZ/Regicide/tree/74e7755470f0fc05cf34d22df3db46e090f45308/experiments/truth-data/matches
[T-38]: https://github.com/AdminGodZ/Regicide/blob/74e7755470f0fc05cf34d22df3db46e090f45308/experiments/truth-data/logs/p5_speed.txt
[T-39]: https://github.com/AdminGodZ/Regicide/tree/74e7755470f0fc05cf34d22df3db46e090f45308/experiments/truth-data/matches
[B-40]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/wp10/games
[B-41]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/wp10/games
[B-42]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/wp10/games
[B-43]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/k3/games
[B-44]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/wp10/games
[M-45]: https://github.com/AdminGodZ/Regicide/tree/33156cf86918ba12680b6305ae039250b2f83786/experiments/time-policy
[M-46]: https://github.com/AdminGodZ/Regicide/blob/33156cf86918ba12680b6305ae039250b2f83786/experiments/time-policy/TIME_POLICY.md
[M-47]: https://github.com/AdminGodZ/Regicide/blob/33156cf86918ba12680b6305ae039250b2f83786/experiments/time-policy/README.md
[M-48]: https://github.com/AdminGodZ/Regicide/blob/33156cf86918ba12680b6305ae039250b2f83786/src/search.cpp
[B-49]: https://github.com/AdminGodZ/Regicide/blob/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/wp11/book_r11.bin
[B-50]: https://github.com/AdminGodZ/Regicide/blob/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/pins.txt
[L-51]: https://github.com/AdminGodZ/Regicide/tree/5f450ef968766cca164255d3b3639085c56c99ec/experiments/search-lab/logs/matches
[L-52]: https://github.com/AdminGodZ/Regicide/blob/5f450ef968766cca164255d3b3639085c56c99ec/experiments/search-lab/RESULTS.md
[L-53]: https://github.com/AdminGodZ/Regicide/tree/5f450ef968766cca164255d3b3639085c56c99ec/experiments/search-lab/logs/ttd
[L-54]: https://github.com/AdminGodZ/Regicide/tree/5f450ef968766cca164255d3b3639085c56c99ec/experiments/search-lab/logs/matches
[B-55]: https://github.com/AdminGodZ/Regicide/tree/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/wp10/games
[M-56]: https://github.com/AdminGodZ/Regicide/blob/33156cf86918ba12680b6305ae039250b2f83786/experiments/booklab/wp12/PROTOCOL.md
[B-57]: https://github.com/AdminGodZ/Regicide/blob/2d85e83eb95d37dbb070c5d51825cc5303c13bef/experiments/booklab/RESULTS.md
[T-58]: https://github.com/AdminGodZ/Regicide/blob/74e7755470f0fc05cf34d22df3db46e090f45308/experiments/truth-data/RESULTS.md
[L-59]: https://github.com/AdminGodZ/Regicide/blob/5f450ef968766cca164255d3b3639085c56c99ec/experiments/search-lab/RESULTS.md
