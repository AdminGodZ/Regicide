# Independent exposure check

This checker is for E/EB/EBN using the current engine, not O's older book policy, and for finished PDNs written by
the current tools/match runner. It does not search. It reads all dealt
and played moves, validates the whole record with both the engine and tools/common, and excludes dealt moves and
the final position from the set of actual engine calls. Unsupported or ambiguous movetext is an error, never a
clean result. It rejects continuation past the runner's repetition, no-progress and maximum-length rules.

Use the actual engine settings. The default assumptions are book enabled in **CB book mode 3**, deterministic
margin zero; no searchmoves/infinite analysis; no additional custom avoid file; database coverage 8 pieces; runner
no-progress 40 moves and max-plies 1000. The two books must have byte-identical avoid lists. `set book 1` is random
margin 60 in this engine and does not meet the deterministic assumption. Nonzero margins need their actual RNG
state for an exact probe comparison. Actual game histories and settings are still required; this tool cannot
recover an omitted move or an unknown engine option from a result CSV.

At the position **before each recorded engine move**, classify potential exposure iff all of these hold:

1. It is our side's turn, after all dealt opening plies and before the end of the record.
2. The position's full book entry list differs between the two files. The key includes side to move. The actual
   payload has 975 such keys; 12 of its 987 force decisions already leave the old entry list unchanged.
3. Book consultation is enabled, the relevant book is nonempty, and this is an ordinary finite move request.
4. The reconstructed reversible-ply count is zero. Every capture or man move, including promotion, resets it;
   each quiet king move increments it. Reconstruct this from the dealt moves as well as the played moves. A FEN
   alone does not describe prior history; the runner starts a fresh history at its initial FEN.
5. The root is not a database **win or loss** under the engine's enabled coverage. These bypass the book, both
   when MTC returns a move and when conversion search takes over. A database draw or Unknown does not disable
   the book. A capture pending does not disable the book either.

The first exposing ply is one-based in the full PDN movetext, including the dealt prefix. Thus the root before
the fifth action reports ply 5, not ply 4. `first_probe_change_ply` is the first eligible root whose deterministic
hit/miss or full selected Move differs. This is stronger than raw-entry exposure: in this payload 975 decision
roots change entries, while 958 change the margin-zero probe result. The other 17 are conservatively called
exposed even though the deterministic selected move stays the same.

`unknown` means an eligible changed root lacked its history or database value; it never means unexposed. If a
later known exposure exists, the classification is exposed, but an earlier `earliest_unknown_ply` makes the exact
first exposure unresolved. With the actual payload every changed root has at least nine pieces, so an 8-piece
database cannot suppress a changed-root consultation and no tables are needed to classify these panels.

The optional `--played-book old|new` additionally checks that any hit at an eligible changed root agrees with the
recorded full move for the named arm. Use `new` for EB/EBN and `old` for E. A mismatch is an error, not evidence of
an unexposed game. Keep the TSV output and run configuration with the match evidence.

## Build and run

Use the final source tree and an ordinary CMake build. The helper and Python wrapper can be put together in
experiments/holdout/. Link against the independent referee as well as the engine:

```sh
clang++ -std=c++26 -O2 -I "$SRC/src" -I "$SRC/tools/common" exposure_helper.cpp \
  "$BUILD/libregicide_core.a" "$BUILD/libregicide_egdb.a" "$BUILD/tools/libcbt_common.a" \
  -pthread -o exposure_helper
python3 classify_exposure.py --helper ./exposure_helper \
  --old-book old_book.bin --new-book book_booklab.bin \
  --our-name 'Regicide EB' --played-book new games.pdn > exposure.tsv
```

`--our-name` is an exact Black/White tag, or use `--our-color B|W`. Opening tags supply the dealt-ply count and their
prefix must agree with movetext. If tags are unavailable, supply `--opening-plies N`. Use `--db-pieces 0` for a run
without the database; `--egdb DIR` supplies a local oracle when a different pair of books changes covered roots.
Only 16 MB is prefaulted. Override runner stopping settings with `--no-progress` and `--max-plies` if necessary.

The C++ helper also accepts TSV records directly: id, B/W, dealt plies, initial reversible plies or -1 when unknown,
initial FEN or -, move tokens. It prints both exposure columns and any earliest unknown, and validates every
record before classifying it. It supports an empty played sequence. Per-record errors appear as `error` rows;
the caller must count and resolve those, not omit them.

## Interpretation

Exposure is a necessary-condition diagnostic. Under coupled deterministic execution, E and EB cannot first
diverge through book selection outside the changed, eligible roots. The wall-clock match plan does not guarantee
identical independent trajectories or identical unexposed outcomes. Search timing and opponent randomness still
vary.

Selecting E follow-ups only because an EB game was exposed selects a group using one arm's realised trajectory.
The resulting subset is descriptive; it does not identify the whole-panel EB-E increment. For that increment,
run E on all of H3, or preselect a random control sample with known sampling weights. Keep all assigned games in
the O-versus-EB and EB-versus-EBN comparisons, and use the frozen H2x tiers for the primary book comparison. Do
not substitute an EB outcome for the unplayed E outcome merely because EB was unexposed.
