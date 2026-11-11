# Brief for an external deep analysis of Regicide (state of master on 2026-11-11: book r11c, net n8q)

This is the briefing for a reviewer who has the repository and time to think, but no access to our machines.
The question is at the end; the sections before it say what is known, with numbers, so that no time goes into
re-deriving it. `docs/EVIDENCE.md` holds the older claims with their sources and conditions; this file is newer
where the two differ.

## What Regicide is

An English (8x8) checkers engine in C++26: bitboards, principal-variation search with the usual reductions and
extensions (`src/search.cpp`), a transposition table (`src/tt.cpp`), an NNUE evaluation (`src/nnue.cpp`, net n8q:
128 inputs, feature transformer 1024, two hidden layers, 8 output buckets by piece count), an embedded opening book
with an avoid list (`src/book.cpp`, `src/weights/book.bin`), an endgame database for 2 to 8 pieces with
win/loss/draw values to 8 pieces and distance values to 6 (`src/egdb/`), and a conversion search for database
wins without distance values. It plays under CheckerBoard as a DLL (`src/cb_api.cpp`).

## The goal and the opponents

Beat, not only hold, the three strongest available programs: Cake 1.89f, KingsRow 1.20 and GuiNN 2.06. Fixed
conditions: the 156 ACF three-move ballots, both colours (312 games per run), 0.5 s per move with CheckerBoard's
time bank, every engine with its own book and an 8-piece database, one thread. Decisions that stand: no 9-piece
database; more threads and pondering are allowed (tested, see below, and not used); book lines learned from games
against these opponents are allowed. The engine cannot know which opponent it faces, so the book is keyed by position only.

## Where we stand (wins/draws/losses for Regicide)

Built-in book r11c with the built-in net n8q and with the previous net n6c, two runs each against Cake and
KingsRow, one against GuiNN (`t3/booklab`, `experiments/booklab/wp11c/`):

| Opponent | n8q (built in) | n6c |
|---|---|---|
| Cake 1.89f | 0/312/0 and 0/312/0 | 0/312/0 and 0/312/0 |
| KingsRow 1.20, book mode 3 ("best moves") | 1/311/0 and 1/311/0 | 0/312/0 and 1/311/0 |
| GuiNN 2.06 | 23/289/0 | 21/291/0 |

No loss in 3,120 games.

For scale: Cake against KingsRow in book mode 3 on the same deck scores 1/311/0. On these openings the top
programs draw almost everything with each other, and so do we now.

On 156 fresh, unbalanced four-ply openings that no book covers (holdout panel H3, `experiments/holdout/`) we are
level with Cake: 57/196/59 (the engine of two days earlier: 14/240/58). Almost all decisive games there are decided
by the opening itself.

## How we got here, in one paragraph each

- Endgame: the old engine converted about a quarter of the database wins it reached; the conversion search converts
  all of them. This is the whole gain on fresh openings.
- Book: every loss on the standard deck was traced to a specific move, usually with 20 to 24 pieces on the board.
  A repair takes the move that at least two of three sources agree on (the move Cake played in drawn games, the
  opponent's own book move with colours as in the game, our 20M-node search) and is accepted after targeted games
  against all three opponents. On top of the repairs, 45,916 "pins" fix our move at every position met in 4,992
  validated games, never a move from a lost game that is still playable under the book. Before the pins a single
  new loss appeared about once in 400 games, each on a different opening, because our search does not choose the
  same move every time; with the pins there was no loss in 1,560 games.
- KingsRow's book mode matters: against its default mode 1 ("all kinds of book moves") we scored 23/601/0, against
  mode 3 the old book lost 7 of 624. All results above are against mode 3.

## What was measured and ruled out

- Time allocation: letting the search use about 70% instead of 44% of the offered time with 21 to 24 pieces changed
  nothing measurable (Cake on H3 +2.5 points of 312, 95% interval -0.5 to +6.0; KingsRow -1.0, -2.5 to 0.0).
- Deeper search in the opening phase: at positions from games against Cake, a 20M-node search picks a different
  move than a 1M-node search in about one position of six at plies 8 to 14, but the difference exceeds 15
  hundredths of a man in about 4% of positions.
- One step above the database the search is nearly error-free from 20k nodes on; no lost game turned with 8 or
  fewer pieces. Lost games are proven lost at a median of 10 pieces but turn much earlier.
- Nets: relabelling training data with database truth (n8q, now built in) gives +3.5 Elo over n6c in self-play
  with the database (interval +0.1 to +6.8) and +0.6 without (-3.1 to +4.4). A net with 15M more database-true self-play records (n9h)
  scores proven wins and losses near the database a little more decisively and judges draws above the database no
  better. Wider feature transformers were slower and not stronger at short time. None of these nets changes a
  result against the three opponents. Before the pins, n8q after the same book lost 5 of 312 to Cake where n6c lost
  none, because the zero-loss result rested on the moves the net chose after the book; with the pinned book both
  nets are clean.
- Search ideas (`t3/search-lab`, `experiments/search-lab/RESULTS.md`): seven switches (hash-first database
  probes, a leaf evaluation cache, correction history, an extension change, double and negative singular
  extensions, the stored evaluation from the hash table) screened in self-play at 300 ms over 800 pairs from an
  unbalanced opening set were all within 3.5 Elo of zero; a node-based time rule passed the screen at +5.0 and
  failed its confirmation. Earlier late-move-reduction results were void (one table shared by both engines of a
  process, since fixed); that sweep is being redone.
- Threads and pondering: at 4 threads the search reaches the same depth 2.8 times faster; three ways of combining
  the threads' results did not beat the plain scheme; and 4 threads plus pondering gave no gain against Cake in
  1,248 games.

## What the wins look like

- Against GuiNN we win 16 to 21 games per run; GuiNN also loses 21 of 40 games to KingsRow on the five hardest
  ballots.
- Against KingsRow in mode 3 we have a handful of wins on record (ACF 71 as White twice, ACF 43 as White, a few in
  the earlier runs). They come from KingsRow varying a move after its book ends.
- Against Cake we have one win in the standard runs (ACF 163, Cake Black). Its first clearly worse move in that
  game is at ply 51 with 13 pieces, and none of Cake's other games of that opening reaches that position. But when
  that opening and colour was replayed 20 times, we scored 6/14/0: Cake does not repeat its own play, and one of its
  three choices at plies 15 to 19 (10-15 at ply 15, 9 games) lost 6 times.
- Cake repeats about 17% of its games move for move against the same play; KingsRow in mode 3 repeats its opening
  play closely and varies later (median first difference between two runs at ply 42).

## The tools you can read

`tools/match` and `tools/cbhost` (the referee and the host that loads CheckerBoard DLLs), `tools/selfplay.cpp`,
`tools/bookgen.cpp` and `tools/lossprobe.cpp` (book construction), `tools/datagen.cpp`, `tools/truth.cpp`,
`tools/rescore.cpp` and `tools/nnue/` (training data and trainer), and on branch `t3/booklab` the book laboratory
(`tools/booklab/`, `experiments/booklab/RESULTS.md`, the win-mining protocol in
`experiments/booklab/wp12/PROTOCOL.md`).

## The question

We no longer lose on the standard deck, and we do not win against Cake or KingsRow. Given everything above:

1. What are the realistic ways to score wins against Cake 1.89f and KingsRow 1.20 under these conditions? Consider
   at least: steering into lines where their evaluation or book is known to be wrong; choosing, among moves our
   search rates equal, the one that leaves the opponent the most ways to go wrong; a contempt or "play for
   complications" term; using the 8-piece database more aggressively above 8 pieces (searching for forced entries
   into won database positions); longer forcing analysis offline that the book then replays.
2. For each, say what evidence in this repository supports or contradicts it, what the smallest experiment is that
   would settle it under our conditions (312 games take about 15 minutes on our match machine; engines can be
   rebuilt at will), and what the expected size of the effect is.
3. Is there a structural weakness in the engine as it is written (search, evaluation, time use, database use,
   book handling) that these results hide, and that an opponent with a deeper book or more time would expose?
4. The pins make the standard deck safe by repeating known games. What does that hide, and how should strength on
   positions outside the book be measured so that we are not fooled by our own preparation?

Answers that name a file and function, a concrete position, or a concrete experiment are worth more than general
advice. Where a claim above looks wrong or too convenient, say so and say which record would show it.
