# Reviewer B, round 3

2026-11-11 15:45 UTC

Status: FINAL at the lead request. Task1 and task3 complete; own rank23 through ply134 (next136), predecessors through rank23 (next24), mining through entry34 with entry35 partial (next forced4-8 at100M/d29). No further round today.

## Inputs and execution

Requested input master 8de39d0ca7db3d586cf92b56df25b06a4ae21d81; booklab e2c16559a72eccd17488d2f23c930f5b919a42bf. The original task-1 queries use the unchanged scoring path compiled at 1c6e997. After the fdd154e and 7f3ac69 updates, the core and drivers were freshly rebuilt. Worker search/iteration, evaluation, move generation, TT, EGDB and the net are unchanged; the added claim methods are not called by this driver, and the PR #81 table-drawn-root filter is outside the task-1 roots, which all exceed eight pieces. Full100M ply67 verification on each new build matches every completed row, including nodes, table hits and PV. Query manifests preserve the earlier measurement revisions; new task2 queries carry their core revision in a header. The one cached table-drawn task2 endpoint was refreshed. See current-core/source-audit.txt, validation.txt, pr81-validation.txt and pr81-endpoint-refresh.txt. Built-in n8q SHA-256: 681549690f165eed94edafd958b06f2f76cf44ab5fba6654c82eab183efe9a24.

All searches are sequential, nice -n 19, one thread, new Engine and cleared 128 MiB hash per query, book and avoid off, full record history replayed. WLD coverage is eight pieces; MTC coverage six. Ordinary queries use at most100M actual nodes; the lead explicitly authorized400M for the decisive positions this round. Caps of100M and400M are reduced by1024 internally to avoid node-poll overshoot. Each query has a separate780-second ceiling; each foreground command is limited to840seconds. No match, opponent program, protected directory/process/lock or writable access to the shared database is used.

Latest fetch: master 7f3ac697bc6dc69630b3b55bad92ad00e428aa4c, booklab 9f845d2a7d42f3df19ccd6a187243bbc6a280d45. The specified mo1 PDNs remain unchanged. The claims update and PR #81 stopped-call fix were inspected, and the latest ledger was read. PR #81 changes the table-drawn normal-root filter, outside the task-1 measured roots; Worker search remains byte-identical. After rebuilding on 7f3ac69, the full ply67 100M trace again matched row for row. Task2 refreshes its one affected table-drawn endpoint; exact source provenance is retained.

## Task 1: strict-run loss

Source: experiments/booklab/tests/mo1-kingsrow/games.pdn.gz, round 5.1, ACF 6, Regicide Black, KingsRow White. The record has EngineGamesA=8 and EngineGamesB=8, game 9 in the concurrency-1 run. Its complete legal replay and input are preserved in source-game.txt and replay-input.tsv.

### Database boundary

The score -19973 at ply 103 in the match comment is a searched database-loss score with **ten pieces** still present. It is not a direct root WLD probe. The first directly covered board is White ply 130, W:WK2,K18,K22,32:BK6,K21,28,K29: WLD Win for White (1). After its forced 2x9, Black ply 131 is B:WK9,K18,K22,32:BK21,28,K29: WLD Loss for Black (3). Every legal Black move there has a White-win child. At 13:56 CET the lead shortened the required own-move walk to plies 51–71. Those eleven roots were all measured; roots 73–103 had already finished and are retained separately as additional measurements. The current 100M search first reaches the known-loss score class at 85. Direct WLD probes at 130–131 are separate from a search backed by table leaves. Exact table values use the side-to-move perspective; they do not certify conversion within every possible no-progress window.

Post-book root progress: 11 of 11 own roots (51–71) completed. The final holding-position verdict uses the completed forced comparisons below.

The two drawn mo-kingsrow-a-h1 and mo-kingsrow-b-h1 game-9 records were independently parsed from the current committed PDNs: both share the loss’s first 49 plies, play White 7-3 at 50, have process ages 0/0, and first search at Black 55 with comment score -78. These are record checks, not new engine measurements. See drawn-branch-record-check.tsv. Thus the pre-50 book repair matters independently of which White move appears at 50; no live replay or match is used.

### Per-ply results

Finite scores are hundredths of a man from the side to move. A searched known-loss score class is distinct from a direct table probe. A draw-like score is an engine opinion, not a proof that the position draws. The pair columns are played / alternative at equal completed depth; they come from the 20M and 100M unrestricted depth references. If a restricted move cannot finish the reference depth, the largest common completed depth is shown. Root scores are last completed iterations; final partial status remains in raw traces. A sole legal move can trigger the ordinary shallow shortcut because of the independent time ceiling; those roots are additionally searched as a restricted sole-move query for the full 100M budget, shown in the forced column.

| Ply | FEN | Pieces | Played | 20M root | 100M root | 20M pair | 100M pair | 400M pair | 400M root | Direct WLD | Verdict |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 17 | B:W19,21,22,23,27,28,30,31,32:B2,4,5,6,7,8,12,13,14 | 18 | 6-10 | 6-10 / -84 / d25 | 8-11 / -27 / d29 | -74 / -42 at d25 (alternative minus played 32) | -87 / -26 at d29 (alternative minus played 61) | -67 / -19 at d34 (alternative minus played 48) | 8-11 / -10 / d34 | Unknown (>8 pieces) | latest book alternative passing -30 at 100M, confirmed at 400M |
| 19 | B:W19,21,22,23,24,27,30,31,32:B2,4,5,7,8,10,12,13,14 | 18 | 14-17 | - | 14-17 / -69 / d31 | - | - | - | - | Unknown (>8 pieces) | negative search opinion |
| 21 | B:W14,19,22,23,24,27,30,31,32:B2,4,5,7,8,10,12,13 | 17 | 10x17x26 | - | 10x17x26 / -10 / d1 | - | - | - | - | Unknown (>8 pieces) | only legal move; shallow shortcut |
| 23 | B:W19,22,23,24,27,30,32:B2,4,5,7,8,12,13 | 14 | 7-10 | - | 7-10 / -75 / d33 | - | - | - | - | Unknown (>8 pieces) | negative search opinion |
| 25 | B:W18,19,22,24,27,30,32:B2,4,5,8,10,12,13 | 14 | 10-14 | - | 10-14 / -95 / d35 | - | - | - | - | Unknown (>8 pieces) | negative search opinion |
| 27 | B:W9,19,22,24,27,30,32:B2,4,5,8,12,13 | 13 | 5x14 | - | 5x14 / -75 / d1 | - | - | - | - | Unknown (>8 pieces) | only legal move; shallow shortcut |
| 29 | B:W19,22,23,24,30,32:B2,4,8,12,13,14 | 12 | 14-17 | - | 14-17 / -114 / d35 | - | - | - | - | Unknown (>8 pieces) | negative search opinion |
| 31 | B:W18,19,23,24,30,32:B2,4,8,12,13,17 | 12 | 17-22 | - | 17-22 / -105 / d35 | - | - | - | - | Unknown (>8 pieces) | negative search opinion |
| 33 | B:W14,19,23,24,30,32:B2,4,8,12,13,22 | 12 | 13-17 | - | 13-17 / -109 / d34 | - | - | - | - | Unknown (>8 pieces) | negative search opinion |
| 35 | B:W9,19,23,24,30,32:B2,4,8,12,17,22 | 12 | 17-21 | - | 17-21 / -108 / d34 | - | - | - | - | Unknown (>8 pieces) | negative search opinion |
| 37 | B:W5,19,23,24,30,32:B2,4,8,12,21,22 | 12 | 2-7 | - | 2-7 / -108 / d34 | - | - | - | - | Unknown (>8 pieces) | negative search opinion |
| 39 | B:W5,18,19,24,30,32:B4,7,8,12,21,22 | 12 | 22-25 | - | 22-25 / -129 / d35 | - | - | - | - | Unknown (>8 pieces) | negative search opinion |
| 41 | B:W5,15,18,24,30,32:B4,7,8,12,21,25 | 12 | 7-11 | - | 7-11 / -114 / d34 | - | - | - | - | Unknown (>8 pieces) | negative search opinion |
| 43 | B:W5,10,18,24,30,32:B4,8,11,12,21,25 | 12 | 11-16 | - | 11-16 / -130 / d33 | - | - | - | - | Unknown (>8 pieces) | negative search opinion |
| 45 | B:W5,7,18,24,30,32:B4,8,12,16,21,25 | 12 | 25-29 | - | 16-20 / -135 / d32 | - | - | - | - | Unknown (>8 pieces) | negative search opinion |
| 47 | B:W5,7,14,24,30,32:B4,8,12,16,21,K29 | 12 | 16-20 | - | 16-20 / -94 / d32 | - | - | - | - | Unknown (>8 pieces) | negative search opinion |
| 49 | B:W5,7,14,19,30,32:B4,8,12,20,21,K29 | 12 | 29-25 | - | 29-25 / -80 / d31 | - | - | - | - | Unknown (>8 pieces) | negative search opinion |
| 50 | W:W5,7,14,19,30,32:B4,8,12,20,21,K25 | 12 | 19-15 | 5-1 / 77 / d25 | 5-1 / 87 / d30 | 92 / 71 at d25 (alternative minus played -21) | 86 / 46 at d29 (alternative minus played -40) | - | - | Unknown (>8 pieces) | favorable search opinion |
| 51 | B:W5,7,14,15,30,32:B4,8,12,20,21,K25 | 12 | 25-22 | - | 25-22 / -83 / d32 | - | - | - | 25-22 / -106 / d36 | Unknown (>8 pieces) | negative search opinion |
| 53 | B:W5,7,10,14,30,32:B4,8,12,20,21,K22 | 12 | 8-11 | - | 8-11 / -24 / d31 | - | - | - | - | Unknown (>8 pieces) | draw-like search opinion |
| 55 | B:WK1,7,10,14,30,32:B4,11,12,20,21,K22 | 12 | 11-15 | - | 11-15 / -24 / d30 | - | - | - | - | Unknown (>8 pieces) | draw-like search opinion |
| 57 | B:WK1,K3,10,14,30,32:B4,12,15,20,21,K22 | 12 | 15-19 | - | 15-19 / -21 / d31 | - | - | - | - | Unknown (>8 pieces) | draw-like search opinion |
| 59 | B:WK1,K3,7,14,30,32:B4,12,19,20,21,K22 | 12 | 12-16 | - | 12-16 / -20 / d32 | - | - | - | - | Unknown (>8 pieces) | draw-like search opinion |
| 61 | B:WK1,K3,7,9,30,32:B4,16,19,20,21,K22 | 12 | 22-26 | - | 22-26 / -19 / d33 | - | - | - | - | Unknown (>8 pieces) | draw-like search opinion |
| 63 | B:WK1,K3,7,9,23,32:B4,16,19,20,21 | 11 | 19x26 | - | 19x26 / -42 / d1 | - | sole/first move: 19x26 / -20 / d32 | - | - | Unknown (>8 pieces) | only legal move; full-budget continuation in forced column |
| 65 | B:WK1,K2,K3,9,32:B4,16,20,21,26 | 10 | 26-30 | 20-24 / -20 / d29 | 20-24 / -20 / d32 | -19 / -21 at d29 (alternative minus played -2) | -18 / -18 at d32 (alternative minus played 0) | - | - | Unknown (>8 pieces) | draw-like search opinion |
| 67 | B:WK1,K3,K7,9,32:B4,16,20,21,K30 | 10 | 20-24 | 30-26 / -19 / d28 | 30-26 / -18 / d31 | -20 / -20 at d28 (alternative minus played 0) | -129 / -18 at d29 (alternative minus played 111) | -210 / -17 at d34 (alternative minus played 193) | 30-26 / -17 / d37 | Unknown (>8 pieces) | last identified holding root through 71; played below -60, alternative within -30 at 100M/400M |
| 69 | B:WK1,K3,9,K11,32:B4,16,21,24,K30 | 10 | 16-19 | - | 16-19 / -200 / d34 | - | - | - | - | Unknown (>8 pieces) | negative search opinion |
| 71 | B:WK1,K3,9,K15,32:B4,19,21,24,K30 | 10 | 19-23 | - | 19-23 / -155 / d33 | - | - | - | - | Unknown (>8 pieces) | negative search opinion |

### Additional roots completed before the shortened instruction

These 100M roots had already finished before the lead shortened the remaining range to ply 71. No further restricted comparisons were made for them.

| Ply | FEN | Played | 100M root |
|---|---|---|---|
| 73 | B:WK1,K3,9,K18,32:B4,21,23,24,K30 | 23-26 | 23-26 / -198 / d33 |
| 75 | B:WK1,K3,9,K23,32:B4,21,24,26,K30 | 24-28 | 24-28 / -269 / d32 |
| 77 | B:WK3,K6,9,K23,32:B4,21,26,28,K30 | 26-31 | 26-31 / -259 / d33 |
| 79 | B:WK6,K7,9,K23,32:B4,21,28,K30,K31 | 4-8 | 31-26 / -252 / d29 |
| 81 | B:WK7,9,K10,K23,32:B8,21,28,K30,K31 | 21-25 | 31-26 / -294 / d30 |
| 83 | B:WK7,9,K15,K23,32:B8,25,28,K30,K31 | 25-29 | 8-12 / -471 / d33 |
| 85 | B:WK7,9,K15,K19,32:B8,28,K29,K30,K31 | 31-26 | 30-26 / -19963 / d32 |
| 87 | B:W9,K11,K15,K19,32:B8,K26,28,K29,K30 | 8-12 | 8-12 / -19961 / d37 |
| 89 | B:W9,K11,K18,K19,32:B12,K26,28,K29,K30 | 29-25 | 29-25 / -19967 / d38 |
| 91 | B:W9,K11,K18,K24,32:B12,K25,K26,28,K30 | 26-22 | 26-22 / -19965 / d39 |
| 93 | B:W9,K15,K18,K24,32:B12,K22,K25,28,K30 | 22-17 | 22-17 / -19969 / d38 |
| 95 | B:W6,K15,K18,K24,32:B12,K17,K25,28,K30 | 25-21 | 30-26 / -19971 / d37 |
| 97 | B:WK2,K15,K18,K24,32:B12,K17,K21,28,K30 | 30-25 | 30-25 / -19971 / d36 |
| 99 | B:WK7,K15,K18,K24,32:B12,K17,K21,K25,28 | 17-13 | 17-13 / -19973 / d39 |
| 101 | B:WK10,K15,K18,K24,32:B12,K13,K21,K25,28 | 13-9 | 13-17 / -19975 / d39 |
| 103 | B:WK14,K15,K18,K24,32:BK9,12,K21,K25,28 | 9-13 | 9-13 / -19977 / d38 |

### Every legal move after 19-15 (Black ply 51)

| Move | 400M score | Completed depth | Actual nodes | PV prefix |
|---|---|---|---|---|
| 25-22 | -96 | 35 | 399998977 | 25-22 5-1 20-24 15-10 8-11 7-3 11-16 1-5 16-19 5-9 12-16 9-13 22-26 30x23 19x26 13-17 |
| 25-29 | -233 | 35 | 399998977 | 25-29 7-3 29-25 5-1 25-22 1-5 20-24 15-10 8-11 3-8 11-15 8-11 15-18 14-9 18-23 9-6 |
| 20-24 | -104 | 36 | 399998977 | 20-24 7-3 25-22 14-9 22-18 15-10 18-23 5-1 24-27 1-6 27-31 10-7 31-26 7-2 8-11 6-10 |
| 12-16 | -128 | 35 | 399998977 | 12-16 5-1 25-22 7-3 8-12 15-11 16-19 1-6 22-18 14-9 18-23 6-10 19-24 9-6 24-27 3-7 |
| 8-11 | -19950 | 45 | 399998977 | 8-11 15x8 4x11 7-3 25-22 14-10 11-15 3-8 15-18 5-1 18-23 8-11 12-16 1-6 23-27 32x23 |

All five legal moves score below -60 in their completed 400M restricted searches. By the lead’s requested screen, the book continuation to ply 49 should change; this is a search conclusion rather than a solved twelve-piece result.

### Latest book-alternative screen

Each restricted root below contains all legal moves except the recorded book move. This gives the alternatives a fresh 100M query and keeps the latest-repair screen separate from the ordinary unrestricted result. Forced captures have no alternative and are skipped. A qualifying candidate still receives its own forced comparison.

| Ply | FEN | Book move | Unrestricted 100M | Book-excluded 100M | At least -30 |
|---|---|---|---|---|---|
| 49 | B:W5,7,14,19,30,32:B4,8,12,20,21,K29 | 29-25 | 29-25 / -80 / d31 | 8-11 / -81 / d32 | False |
| 47 | B:W5,7,14,24,30,32:B4,8,12,16,21,K29 | 16-20 | 16-20 / -94 / d32 | 29-25 / -247 / d31 | False |
| 45 | B:W5,7,18,24,30,32:B4,8,12,16,21,25 | 25-29 | 16-20 / -135 / d32 | 16-20 / -197 / d32 | False |
| 43 | B:W5,10,18,24,30,32:B4,8,11,12,21,25 | 11-16 | 11-16 / -130 / d33 | 25-29 / -214 / d31 | False |
| 41 | B:W5,15,18,24,30,32:B4,7,8,12,21,25 | 7-11 | 7-11 / -114 / d34 | 25-29 / -217 / d32 | False |
| 39 | B:W5,18,19,24,30,32:B4,7,8,12,21,22 | 22-25 | 22-25 / -129 / d35 | 7-11 / -127 / d34 | False |
| 37 | B:W5,19,23,24,30,32:B2,4,8,12,21,22 | 2-7 | 2-7 / -108 / d34 | 22-25 / -132 / d33 | False |
| 35 | B:W9,19,23,24,30,32:B2,4,8,12,17,22 | 17-21 | 17-21 / -108 / d34 | 2-7 / -188 / d33 | False |
| 33 | B:W14,19,23,24,30,32:B2,4,8,12,13,22 | 13-17 | 13-17 / -109 / d34 | 2-7 / -176 / d34 | False |
| 31 | B:W18,19,23,24,30,32:B2,4,8,12,13,17 | 17-22 | 17-22 / -105 / d35 | 17-21 / -121 / d37 | False |
| 29 | B:W19,22,23,24,30,32:B2,4,8,12,13,14 | 14-17 | 14-17 / -114 / d35 | 2-7 / -182 / d34 | False |
| 25 | B:W18,19,22,24,27,30,32:B2,4,5,8,10,12,13 | 10-14 | 10-14 / -95 / d35 | 2-7 / -19968 / d35 | False |
| 23 | B:W19,22,23,24,27,30,32:B2,4,5,7,8,12,13 | 7-10 | 7-10 / -75 / d33 | 5-9 / -209 / d32 | False |
| 19 | B:W19,21,22,23,24,27,30,31,32:B2,4,5,7,8,10,12,13,14 | 14-17 | 14-17 / -69 / d31 | 7-11 / -224 / d29 | False |
| 17 | B:W19,21,22,23,27,28,30,31,32:B2,4,5,6,7,8,12,13,14 | 6-10 | 8-11 / -27 / d29 | 8-11 / -26 / d29 | True |

### Book moves 41–49 and the recorded continuation

The last column fixes **all** recorded moves from the specified book move through White 50 (19-15), including the later own book pins, then evaluates Black 51 with the full original history. Thus its endpoint evaluation is the same at every starting ply. This is explicitly a prescribed-line value, not an unrestricted minimax score of that earlier root. The lead accepted the repair checkpoint and shortened the remaining work; the displayed forced prefix is the requested recorded book continuation, not a one-move-only restriction.

| Ply | FEN | Book move | 100M root | Fixed endpoint 100M / 400M (Black) | Prescribed line |
|---|---|---|---|---|---|
| 41 | B:W5,15,18,24,30,32:B4,7,8,12,21,25 | 7-11 | 7-11 / -114 / d34 | -83 / -106 | 7-11 15-10 11-16 10-7 25-29 18-14 16-20 24-19 29-25 19-15 |
| 43 | B:W5,10,18,24,30,32:B4,8,11,12,21,25 | 11-16 | 11-16 / -130 / d33 | -83 / -106 | 11-16 10-7 25-29 18-14 16-20 24-19 29-25 19-15 |
| 45 | B:W5,7,18,24,30,32:B4,8,12,16,21,25 | 25-29 | 16-20 / -135 / d32 | -83 / -106 | 25-29 18-14 16-20 24-19 29-25 19-15 |
| 47 | B:W5,7,14,24,30,32:B4,8,12,16,21,K29 | 16-20 | 16-20 / -94 / d32 | -83 / -106 | 16-20 24-19 29-25 19-15 |
| 49 | B:W5,7,14,19,30,32:B4,8,12,20,21,K29 | 29-25 | 29-25 / -80 / d31 | -83 / -106 | 29-25 19-15 |

### KingsRow ply 50, White view

Root20: 5-1 / 77 / d25; root100: 5-1 / 87 / d30. 19-15 / 7-3: 92 / 71 at d25 (alternative minus played -21) at the shallow reference, 86 / 46 at d29 (alternative minus played -40) at the deep reference. The advantage of 19-15 over 7-3 is 21 / 40 at those equal-depth references. Positive scores favor White.

### Repair proposal

At Black ply 17, FEN **B:W19,21,22,23,27,28,30,31,32:B2,4,5,6,7,8,12,13,14**, propose **8-11** in place of the book’s **6-10**. This is the latest point meeting the -30 screen: every later tested book alternative through ply 49 misses that limit, and plies 21 and 27 have only one legal move. The 100M root chooses 8-11 at -27/d29; the 400M root chooses it at -10/d34, and its forced 400M run finishes at -21/d35. Equal-depth book/alternative scores are -74/-42 at d25 (20M reference), -87/-26 at d29 (100M reference), and -67/-19 at d34 (400M reference). A legal 400M root line begins **8-11 28-24 6-9 22-18 7-10 30-26 2-7 24-20**. This is search evidence on an eighteen-piece position, not a solved draw; no book file has been changed.

### Last holding position in the shortened walk

The last identified holding root through ply 71 is **Black ply 67**, FEN **B:WK1,K3,K7,9,32:B4,16,20,21,K30**, with **30-26** instead of the played **20-24**. The unrestricted root chooses 30-26 at -18/d31 with 100M and -17/d37 with 400M. Equal-depth played/alternative scores are -20/-20 at d28 (20M reference), -129/-18 at d29 (100M reference), and -210/-17 at d34 (400M reference). The forced 400M runs finish at -248/d37 for 20-24 and -17/d34 for 30-26. Thus the shallow reference does not separate the moves, while both deeper references strongly prefer the holding candidate. This is a ten-piece search opinion, not an exact root WLD draw. The common depths 29 and 34 are below the unrestricted reference depths because the restricted 30-26 search cannot complete those deeper iterations within the caps.

At ply 65, the played 26-30 and the alternative 20-24 both score -18 at equal depth 32 with 100M. At the forced ply 63, the sole move 19x26 scores -20 after a full-budget restricted search. These do not identify an earlier deciding error. The subsequent roots at plies 69 and 71 choose the played moves at -200/d34 and -155/d33 respectively; no holding choice is identified there by their unrestricted 100M queries. A holding candidate here means an evaluation at least -30 in the deepest relevant query, not a proof of the game-theoretic result.


The recorded continuation is Black51 25-22, White52 15-10, Black53 8-11. The 100M score at ply 53 is -24 and later recorded roots stay near -20 through ply 67. The negative 400M assessment at ply 51 and the later holding candidate describe different positions along this recorded line; no forced-loss proof or separate White52 comparison was run.

## Task2 continuation

The frequency pool is now **8880 games**, including **936 concurrency1 mo1 games** with validated worker/process-age tags. The original116-game rank order is retained. Own-game coverage is through rank **22** (first20 completed in B2); this round has **68 own positions** and **3 predecessors** completed. Mining coverage is through entry **34/36**, with **22 entries** completed this round. New supported contexts: own **0**, predecessors **0**, mining boards **6**.

The [task2 report](task2/REPORT.md) contains the complete new comparisons, updated transposition census including the identifiable C1 subset, reused/new trace manifest and exact remaining queue. Accepted prior points are referenced without another nomination.

Final stop: own rank23 through ply134 (next136); predecessors through rank23 (next24); mining entry34 complete, entry35 partial (next forced4-8,100M,target depth29), entry36 unqueried. The final query finished naturally, and no review search/queue process remains.

## Task 3: stopped-call fallback, reading only

The fallback is main.roots[0].move (current 7f3ac69 src/search.cpp:967; fdd154e:951), initialized in generated root order by Worker::setup:145; egdb_root_move declines drawn roots, and before PR #81 only won roots were filtered, explaining the A6 first-move 25-21 at B:WK30:BK25,K26.

The smallest targeted change removes moves whose child is WLD Win for the opponent at a table-drawn ordinary root before Worker::setup; current master 7f3ac69 (PR #81), src/search.cpp:915-923, now implements it, retains unknown children and keeps the list if filtering would empty it.

Reading only: no stopped-call fixture or patch was run by me; the original execution belongs to A6, and the now-merged fix was checked against the source rather than nominated again.

No patch or fixture execution was performed for this task; the concrete stopped-call result is reviewer A round 6’s executed fixture.

## Evidence and reproduction

per-ply.tsv, book-41-49.tsv, fixed-lines.tsv, ply51-all-moves.tsv and query-manifest.tsv accompany the full query traces in queries/. The complete recorded game and full-history replay input are text files. Only report text is committed under reviews/b/round3.

### replay.cpp

```cpp
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include "search.hpp"
#include "movegen.hpp"
#include "egdb/egdb.hpp"
#include "movetext.hpp"
using namespace regicide;
std::vector<std::string> split(const std::string&s,char c='\t'){std::vector<std::string>v;std::stringstream ss(s);for(std::string x;std::getline(ss,x,c);)v.push_back(x);return v;}
struct Game{std::string id,fen,col;int nopen;std::vector<std::string>mv;};
bool nowdraw(const Position&p,const Move&m,const std::vector<Position>&hist){Position c=p;c.make_move(m);MoveList ml;generate_moves(c,ml);if(ml.empty())return false;int rev=c.reversible_plies();if(rev>=NO_PROGRESS_PLIES)return true;int n=std::min<int>(hist.size(),std::max(rev-1,0)),count=0;for(int i=int(hist.size())-n;i<int(hist.size());++i)count+=hist[i].key()==c.key();return count>=2;}
int main(int ac,char**av){try{
if(ac<3)return 2;std::ifstream in(av[2]);std::map<std::string,Game>gs;
for(std::string l;std::getline(in,l);){auto f=split(l);Game g{f[0],f[1],f[2],std::stoi(f[3]),{}};std::istringstream ms(f[4]);for(std::string m;ms>>m;)g.mv.push_back(m);gs[g.id]=g;}
std::string mode=av[1];
if(mode=="walk"){
 std::cout<<"game\tply\tfen\trev\tplayed\tlegal\tpieces\n";
 for(auto&[id,g]:gs){Position p;if(!g.fen.empty()&&!p.set_fen(g.fen))throw std::runtime_error("fen");int ply=0;for(auto&t:g.mv){Move m;if(!parse_move(p,t,m)||booklab::matching_moves(p,t)!=1)throw std::runtime_error("replay "+id+" "+std::to_string(ply+1));MoveList ml;generate_moves(p,ml);std::cout<<id<<'\t'<<++ply<<'\t'<<p.fen()<<'\t'<<p.reversible_plies()<<'\t'<<move_text(p,m)<<'\t'<<ml.size()<<'\t'<<p.piece_count()<<'\n';p.make_move(m);}}
 return 0;}
if(ac!=9)return 2;auto&g=gs.at(av[3]);int ply=std::stoi(av[4]);uint64_t budget=std::stoull(av[5]);int depth=std::stoi(av[6]);std::string force=av[7];int cap=std::stoi(av[8]);if(budget>400000000)throw std::runtime_error("budget");
if(!egdb::init("/home/admingod/regicide-data/egdb",64))throw std::runtime_error("DB");
Position p;if(!g.fen.empty()&&!p.set_fen(g.fen))throw std::runtime_error("fen");std::vector<Position>hist;
for(int i=0;i<ply-1;++i){Move m;if(!parse_move(p,g.mv.at(i),m))throw std::runtime_error("move");hist.push_back(p);p.make_move(m);}
MoveList ml;generate_moves(p,ml);
std::cout<<"# root "<<p.fen()<<" rev "<<p.reversible_plies()<<" pieces "<<p.piece_count()<<" dbcap "<<cap<<" db "<<(p.piece_count()<=cap?int(egdb::probe(p.pieces(BLACK),p.pieces(WHITE),p.kings(),p.side_to_move())):0)<<"\n";
for(auto&m:ml){Position c=p;c.make_move(m);MoveList replies;generate_moves(c,replies);int db=c.piece_count()<=cap?int(egdb::probe(c.pieces(BLACK),c.pieces(WHITE),c.kings(),c.side_to_move())):0;std::cout<<"# child "<<move_text(p,m)<<" db "<<db<<" drawnow "<<nowdraw(p,m,hist)<<" terminal "<<replies.empty()<<"\n";}
Engine e;e.set_threads(1);e.set_hash_mb(128);e.set_egdb_pieces(cap);e.set_book(nullptr);e.set_avoid(nullptr);e.new_game();e.set_position(p,hist);
Limits l;l.nodes=budget>=100000000?budget-1024:budget;l.depth=depth;l.movetime_ms=780000;l.use_book=false;
if(force!="-"){for(const auto& text:split(force,',')){Move m;if(!parse_move(p,text,m))throw std::runtime_error("force "+text);l.searchmoves.push_back(m);}l.infinite=true;}
std::cout<<"depth\tscore\tmove\tnodes\ttbhits\tpv\n";
auto r=e.go(l,[&](const SearchInfo&i){std::cout<<i.depth<<'\t'<<i.score<<'\t'<<(i.pv.empty()?"none":move_text(p,i.pv[0]))<<'\t'<<i.nodes<<'\t'<<i.tbhits<<'\t';Position q=p;for(auto m:i.pv){MoveList legal;generate_moves(q,legal);if(std::find(legal.begin(),legal.end(),m)==legal.end())throw std::runtime_error("illegal PV");std::cout<<move_text(q,m)<<' ';q.make_move(m);}std::cout<<std::endl;});
std::cout<<"# final depth "<<r.depth<<" completed "<<r.completed_score<<" score "<<r.score<<" move "<<move_text(p,r.best)<<" partial "<<r.partial<<" nodes "<<r.nodes<<" ms "<<r.time_ms<<"\n";
return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 9;}}
```

### task1.py

```python
import csv,json,subprocess,sys,time,re
from pathlib import Path
P=Path('work/round3');D=Path('reports/round3');Q=D/'queries';Q.mkdir(exist_ok=True)
G=json.load(open(P/'loss.json'));W={int(r['ply']):r for r in csv.DictReader(open(P/'walk.tsv'),delimiter='\t')}
def read(p):
 s=p.read_text();lines=s.splitlines();assert lines[-1].startswith('# final'),p
 rows=list(csv.DictReader([l for l in lines if not l.startswith('#')],delimiter='\t'))
 end=lines[-1];n=int(re.search(r'nodes (\d+)',end)[1]);assert n<=400000000,(p,n)
 return dict(rows[-1],path=str(p),final=end)
def query(ply,budget=100000000,move='-',depth=127):
 p=Q/f"P{ply:03}-{budget}-{'root' if move=='-' else 'force-'+move.replace('x','_')}-d{depth}.tsv"
 if p.exists():return read(p)
 tmp=p.with_suffix('.running')
 with tmp.open('w') as f:
  subprocess.run([str(P/'replay'),'query',str(P/'loss.tsv'),'R3L',str(ply),str(budget),str(depth),move,'8'],stdout=f,check=True,timeout=790)
 z=read(tmp);assert int(re.search(r'nodes (\d+)',z['final'])[1])<=budget+(1024 if budget<100000000 else 0)
 tmp.replace(p);z['path']=str(p)
 print('QUERY',ply,budget,move,depth,'=>',z['move'],z['score'],'d'+z['depth'],flush=True)
 return z
if __name__=='__main__':
 start=time.monotonic();mode=sys.argv[1]
 if mode=='roots':
  priority=[51,69,71,67,49,47,45,43,41]+[p for p in range(51,104,2) if p not in (51,69,71,67)]
  for p in priority:
   query(p)
   if time.monotonic()-start>360:print('BATCH CHECKPOINT',flush=True);break
 elif mode=='backtrack':
  for p in range(39,0,-2):
   z=query(p)
   if int(z['score'])>=-30 and int(W[p]['legal'])>1:
    print('REPAIR ROOT CANDIDATE',p,z['move'],z['score'],flush=True)
    (P/'repair-candidate.json').write_text(json.dumps(dict(ply=p,root=z),indent=2))
    break
   if time.monotonic()-start>360:print('BATCH CHECKPOINT',flush=True);break
 elif mode=='alternatives':
  for p in range(49,3,-2):
   if int(W[p]['legal'])==1:continue
   root=query(p)
   moves=re.findall(r'^# child (\S+) ',Path(root['path']).read_text(),re.M)
   alternatives=[m for m in moves if m!=W[p]['played']]
   z=query(p,move=','.join(alternatives))
   if int(z['score'])>=-30:
    print('REPAIR ALTERNATIVE CANDIDATE',p,z['move'],z['score'],flush=True)
    (P/'repair-candidate.json').write_text(json.dumps(dict(ply=p,root=root,alternative=z),indent=2))
    break
   if time.monotonic()-start>360:print('BATCH CHECKPOINT',flush=True);break
 elif mode=='jobs':
  for j in json.load(open(sys.argv[2])):
   query(**j)
   if time.monotonic()-start>360:print('BATCH CHECKPOINT',flush=True);break
 else:raise ValueError(mode)
```

### compare.py

```python
import sys,json,csv,time
from pathlib import Path
sys.path.insert(0,'work/round3')
import task1 as A
P=A.P;D=A.D
def trace(z):
 return {int(r['depth']):r for r in csv.DictReader([l for l in Path(z['path']).read_text().splitlines() if not l.startswith('#')],delimiter='\t')}
def pair(played,alt,reference):
 a,b=trace(played),trace(alt);common=set(a)&set(b);dep=max(d for d in common if d<=reference) if any(d<=reference for d in common) else min(common)
 return dict(depth=dep,played=int(a[dep]['score']),alternative=int(b[dep]['score']),gain=int(b[dep]['score'])-int(a[dep]['score']),played_path=played['path'],alternative_path=alt['path'])
state_path=P/'comparisons.json'
state=json.loads(state_path.read_text()) if state_path.exists() else {}
start=time.monotonic()
def save():state_path.write_text(json.dumps(state,indent=2))
def runpoint(p,alt_override=None,deep=False):
 played=A.W[p]['played'];r=A.query(p);r20=None
 alt=alt_override or r['move']
 key=f'{p}:{played}:{alt}'
 if key in state:return
 if alt!=played:
  r20=A.query(p,20000000);target=max(int(r['depth']),int(r20['depth']))
  zp=A.query(p,move=played,depth=target);za=A.query(p,move=alt,depth=target)
  state[key]=dict(ply=p,played=played,alternative=alt,root100=r,root20=r20,pair20=pair(zp,za,int(r20['depth'])),pair100=pair(zp,za,int(r['depth'])))
 else:
  z=A.query(p,move=played,depth=127 if int(A.W[p]['legal'])==1 else int(r['depth']))
  state[key]=dict(ply=p,played=played,alternative=alt,root100=r,forced100=z)
 save();print('COMPARED',key,state[key].get('pair20'),state[key].get('pair100'),flush=True)
if __name__=='__main__':
 mode=sys.argv[1]
 if mode=='own':
  for p in range(51,72,2):
   rootpath=A.Q/f'P{p:03}-100000000-root-d127.tsv'
   if not rootpath.exists():continue
   r=A.read(rootpath)
   if r['move']!=A.W[p]['played'] or int(A.W[p]['legal'])==1:runpoint(p)
   if time.monotonic()-start>360:print('BATCH CHECKPOINT',flush=True);break
 elif mode=='white50':
  p=50;root=A.query(p);r20=A.query(p,20000000);target=max(int(root['depth']),int(r20['depth']))
  z1=A.query(p,move='19-15',depth=target);z2=A.query(p,move='7-3',depth=target)
  state['white50']=dict(ply=p,played='19-15',alternative='7-3',root20=r20,root100=root,pair20=pair(z1,z2,int(r20['depth'])),pair100=pair(z1,z2,int(root['depth'])))
  save();print(json.dumps(state['white50'],indent=2))
 elif mode=='repair':
  c=json.load(open(P/'repair-candidate.json'));runpoint(c['ply'],c['alternative']['move'])
 elif mode=='deeprepair':
  c=json.load(open(P/'repair-candidate.json'));p=c['ply'];alt=c['alternative']['move'];played=A.W[p]['played']
  r=A.query(p,400000000);za=A.query(p,400000000,alt);zp=A.query(p,400000000,played)
  state['deeprepair']=dict(ply=p,played=played,alternative=alt,root400=r,alternative400=za,played400=zp,pair400=pair(zp,za,int(r['depth'])))
  save();print(json.dumps(state['deeprepair'],indent=2))
 elif mode=='deep67':
  r=A.query(67,400000000);p=A.W[67]['played'];alt=r['move']
  za=A.query(67,400000000,alt);zp=A.query(67,400000000,p)
  state['deep67']=dict(ply=67,played=p,alternative=alt,root400=r,pair400=pair(zp,za,int(r['depth'])))
  save();print(json.dumps(state['deep67'],indent=2))
 else:raise ValueError(mode)
```
