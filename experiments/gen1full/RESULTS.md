# gen1full: bands eval retrained on all gen1 data (B70) vs B13

The engine binaries, the trainer and bookgen were built from `t3/optimize-checkers-engine` @ 2732f0d (Release,
clang 22, `-march=native`, WSL2, Core Ultra 9 285K). That build has bands ON, and its built-in weights are `w_B13.bin`
(= `experiments/eval-gen1/w_B.bin`, trained on 13.3M positions). Nothing in `src/weights/` or the CMake defaults is
changed on this branch.

## Data

| File | Positions | Seed |
|---|---|---|
| `gen1_n10k.bin`  |  9,643,008 | 21 |
| `gen1b_n10k.bin` | 29,817,546 | 22 |
| `gen1c_n10k.bin` | 29,816,510 | 23 |

All three files came from the same datagen command line: 10k nodes, built-in gen0 eval, no egdb.
Train / valid split: 67,891,523 / 1,385,541 positions (trainer seed 7, 2% valid). The fitted K is 98.6.

## Training

```
train --data gen1_n10k.bin --data gen1b_n10k.bin --data gen1c_n10k.bin --out w_B70.bin \
      --epochs 8 --lr 0.01 --batch 65536 --lambda 0.7 --threads 12          # default l2; ~3 min
```

The lr 0.002 and lr 0.0005 runs are controls (same command, different `--lr`, 6 threads). B13 was scored on the
same valid set with `--init w_B13.bin --epochs 0` (`logs/eval_B13_fullsplit.log`).

Valid loss per epoch. Epoch 0 is the material-only init, except for B13, where it is B13 itself.

| Weights | ep0 | ep1 | ep2 | ep3 | ep4 | ep5 | ep6 | ep7 | ep8 (final) | final train |
|---|---|---|---|---|---|---|---|---|---|---|
| B13 (built-in) | 0.535870 | | | | | | | | | |
| **B70** lr 0.01 | 0.538085 | 0.536288 | 0.537770 | 0.538248 | 0.538765 | 0.539278 | 0.539455 | 0.539638 | **0.539824** | 0.539820 |
| B70 lr 0.002   | 0.538085 | 0.536056 | 0.535812 | 0.535793 | 0.535784 | 0.535792 | 0.535798 | 0.535804 | **0.535813** | 0.535898 |
| B70 lr 0.0005  | 0.538085 | 0.536891 | 0.536437 | 0.536342 | 0.536256 | 0.536179 | 0.536156 | 0.536132 | **0.536110** | 0.536210 |

**lr 0.01 diverges on the full data set.** From epoch 2 on, train and valid loss both rise every epoch, even after the
lr steps down to 0.001. The final valid loss is worse than the material-only init. The material values also shrink
(mg man 78 / king 99, against 94 / 124 in B13). There are 5x as many batches per epoch as in eval-gen1, so the
coupled L2 under Adam pulls weights toward zero 5x as often per epoch. 903c510 on `t3/optimize-checkers-engine`
gives the same diagnosis and adds `--wd`. With lr 0.002, the loss stops falling after epoch 4 (0.535784). Its final
loss (0.535813) is only 0.00006 below B13's.

## Match: B70 vs B13

The setup is the same as in `experiments/eval-gen1`:
- Runner: `tools/match` + `cbhost` with `.so` engines, `--movetime 0.1`, `-c 10`.
- Engine settings: `set threads 1`, `set hashsize 128`, `set book 0` for both engines.
- Endgame database: the same 2–6-piece egdb for both, via an `egdb` symlink next to the `.so`.
- Openings: `openings4.txt`, 805 positions x both colours = 1610 games.

The bookgen job (10 threads) was running throughout the match.

| A vs B | Elo [95% CI] (pentanomial) | LOS | W / D / L | pairs LL LD DD+WL WD WW | match nps A / B |
|---|---|---|---|---|---|
| B70 vs B13 | **−1.7** [−10.1, +6.7] | 34.4% | 382 / 838 / 390 | 15 / 144 / 493 / 140 / 13 | 3.65M / 3.70M |

B70 is not stronger than B13, even though B70 had 5x more training data. This fits the diverged training run.
gen2a datagen therefore uses **B13** (the rule was: B70 only if its Elo point estimate is above 0).

## Fix: decoupled weight decay (trainer @ 903c510)

These runs used the trainer from `t3/optimize-checkers-engine` @ 903c510 (`--wd`, `--wd-table`), with coupled
L2 off. The data and split are the same as above.

```
train --data gen1_n10k.bin --data gen1b_n10k.bin --data gen1c_n10k.bin --out w_<run>.bin --epochs 8 --batch 65536 \
      --lambda 0.7 --l2 0 --l2-table 0 --lr <lr> --wd <wd> --wd-table <wd> --threads 4
```

Valid loss per epoch (epoch 0 = material-only init, 0.538085 for every run):

| Run | ep1 | ep2 | ep3 | ep4 | ep5 | ep6 | ep7 | ep8 (final) | final train |
|---|---|---|---|---|---|---|---|---|---|
| lr 0.003, wd 0     | 0.529479 | 0.525727 | 0.524919 | 0.524192 | 0.523549 | 0.523350 | 0.523157 | 0.522971 | 0.523166 |
| lr 0.003, wd 1e-3  | 0.529532 | 0.525809 | 0.525007 | 0.524283 | 0.523645 | 0.523446 | 0.523254 | 0.523069 | 0.523263 |
| lr 0.01, wd 0      | 0.523087 | 0.519787 | 0.519171 | 0.518644 | 0.518200 | 0.518066 | 0.517940 | 0.517819 | 0.517967 |
| lr 0.01, wd 1e-3   | 0.523190 | 0.519911 | 0.519295 | 0.518766 | 0.518320 | 0.518186 | 0.518058 | 0.517936 | 0.518085 |
| **lr 0.03, wd 0**  | 0.518306 | 0.516293 | 0.515973 | 0.515727 | 0.515529 | 0.515469 | 0.515414 | **0.515362** | 0.515437 |
| *extra:* lr 0.1, wd 0 | 0.515444 | 0.514585 | 0.514388 | 0.514275 | 0.514186 | 0.514146 | 0.514121 | *0.514097* | 0.514130 |
| *ref:* B13 (built-in) | | | | | | | | 0.535870 | |
| *ref:* old trainer, lr 0.002 (coupled L2) | | | | | | | | 0.535813 | 0.535898 |

- Removing the coupled L2 is the fix: the valid loss falls from about 0.5358 to 0.515–0.523.
- wd 1e-3 is slightly worse than wd 0, at both lrs.
- Within the sweep, a higher lr is better at every step.
- lr 0.1 was not in the requested sweep. I added it because lr 0.03, the best requested setting, was at the top of
  the range. lr 0.1 is better again (0.514097), and its material values are larger (mg man 123 / king 152). It has
  **not** been matched.
- Material values (mg man / king): B13 94 / 124, lr 0.01 115 / 149, lr 0.03 121 / 154. The old coupled L2 was also
  shrinking material.

### Match: W_lr0.03_wd0 vs B13

The best requested run, lr 0.03 / wd 0, was compiled into a `regicide_cb` `.so` (`w_lr0.03_wd0.bin` copied to
`src/weights/eval.bin` in a scratch checkout of 903c510). The match settings are identical to B70 vs B13. The match ran
after bookgen and gen2a had finished, so the machine was otherwise idle.

| A vs B | Elo [95% CI] (pentanomial) | LOS | W / D / L | pairs LL LD DD+WL WD WW | match nps A / B |
|---|---|---|---|---|---|
| **W_lr0.03_wd0 vs B13** | **+163.7** [+152.4, +175.3] | 100.0% | 888 / 541 / 181 | 0 / 7 / 272 / 338 / 188 | 4.14M / 4.24M |

**Best weights: `wd/w_lr0.03_wd0.bin`** (matched, +164 Elo vs B13). `wd/w_lr0.1_wd0.bin` has an even lower valid
loss but has not been matched.

## gen2a datagen

gen2a used the same command line as gen1, with seed 31, 300k games and 12 threads. It ran on the 2732f0d datagen with
`--evalfile` = B13, because B70 lost the B70 vs B13 match.
Output: `~/regicide-data/train/gen2a_n10k.bin`, **298,399 games, 30,060,997 positions**
(19:25 to 20:57, 92 min). It was started before the wd fix existed, so it was generated with B13, not with the +164
weights.

## Opening book (bookgen, B13 built-in eval)

```
bookgen --nodes 2000000 --expansions 40000 --threads 10 --out book_b13.bin       # 2732f0d, built-in B13, no --evalfile
```

- Runtime: 17:38 to 21:03 (3 h 25 min, 10 threads). The B70 match, the trainings and gen2a shared the machine for
  most of this time.
- Roots: 217 positions (the start position plus every distinct position after 3 plies).
- Result: 40,000 expansions, **40,217 book positions**. Final `next priority` 100, start-position value 0.
- File: `book_b13.bin`, 3,411,804 bytes. The progress log is `logs/bookgen.log`.

## Files

- `w_B70.bin`: the B70 run as specified (lr 0.01). `w_B70_lr0.002.bin`, `w_B70_lr0.0005.bin`: the lr controls.
  `w_B13.bin`: a copy of the built-in weights.
- `wd/`: the weights from the wd sweep (`w_<run>.bin`), including the extra lr 0.1 run.
- `logs/`: the training logs and B13's valid loss on the full split. `logs/wd/` holds the sweep logs.
  `logs/bookgen.log` is the bookgen progress log.
- `matches/B70-vs-B13/` and `matches/W_lr0.03_wd0-vs-B13/`: `summary.txt`, `results.csv`. The PDNs (~16 MB each)
  are not committed.
- `pipeline.sh`: the job script (train -> B13 eval -> B70 `.so` build -> match -> gen2a datagen).
  `sweep.sh`, `after_sweep.sh`: the wd sweep, the lr 0.1 extra run and the W match.
- `book_b13.bin`: the opening book.
