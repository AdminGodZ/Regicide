# book-n4c: opening book from the NNUE engine (n4c)

- **`book_n4c.bin`:** an opening book of **30,217 positions** (2.97 MB, RGBK) generated with master `cc9f35f`. That
  build has the n4c FT 1024 net built in, se_depth 4, se_margin 20, and bench 7,338,658.
- **`book_n4c_avoid.bin`:** the same book with the avoid list `avoid_v1.bin` merged in (`origin/t3/avoid-v1`,
  79 entries). It has **30,158 positions plus 79 avoid entries** (RGBK version 2) and is what the book+avoid test
  embeds. In the merge, avoided moves and the 59 positions reachable only through them were dropped.
- Neither book is embedded in master. Testing is in `results/book-avoid-*` on `t3/book-avoid-test`.

## Generation

```
bookgen --nodes 1000000 --expansions 30000 --threads 10 --out ~/regicide-data/book/book_n4c.bin     # nice 5
```

- **Settings:**
  - `bookgen` from `cc9f35f` (Release, clang, `-march=native`), built-in n4c, default drop and max-ply (`scripts/`).
  - 1M nodes per analysed move, because the NNUE engine is about 2× slower than the linear eval used for the earlier
    books.
  - Roots: the start position and every distinct position after 3 plies (all 3-move ballots); 217 roots.
- **Result:** start value +6 (11-15 is the book move from the start position, played instantly through the
  CheckerBoard DLL).
- **Time:** 15:15–19:43 wall (4 h 28 min); **141 min of actual running**, about 213 expansions/min.
  - Bookgen was paused (SIGSTOP) during every A/B match and CheckerBoard baseline that had priority
    (`logs/timeline.txt`).
  - While running, it shared the machine with the 8-piece EGDB build (8 threads) and with NNUE training on the GPU
    (about 1–2 cores). From 18:25 it ran alongside the book/avoid CheckerBoard tests.
- **Book+avoid merge** (`scripts/book_post.sh`, log `logs/book_avoid_merge.log`):

```
bookgen --resume book_n4c.bin --avoid avoid_v1.bin --expansions 0 --nodes 1000000 --threads 10 --out book_n4c_avoid.bin
```

  - This used `bookgen` from master `65f7143`, which supports `--avoid`.
  - The resume walks the book from the roots using only non-avoided moves. So avoided moves, and every position only
    reachable through them, are dropped, and the 79 avoid entries are written into the file.

## Files

- `book_n4c.bin`, `book_n4c_avoid.bin`
- `logs/`:
  - `book_n4c.log`: bookgen output;
  - `book_avoid_merge.log`;
  - `timeline.txt`: start, pauses, resumes, end;
  - `sha256.txt`.
- `scripts/`:
  - `book_build.sh`: builds bookgen at cc9f35f;
  - `book_n4c.sh`: the run;
  - `book_post.sh`: the merge and the test DLL build.
