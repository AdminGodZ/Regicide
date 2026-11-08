# NNUE trainer

PyTorch trainer for Regicide's NNUE evaluation (`src/nnue.hpp`). It reads the same `DataRecord` files as
`tools/train.cpp` (datagen output) and writes RGNN nets that the engine loads with `set evalfile`.

| File | Purpose |
|---|---|
| `nnue.py` | Architecture, data loading, features, model, quantisation, RGNN read/write, integer reference |
| `train.py` | Training: AdamW, warmup + cosine LR, validation, per-epoch checkpoints and exported nets, resume |
| `export.py` | Checkpoint to RGNN |
| `check_export.py` | Exports a net and checks that Python's integer forward pass equals `regicide evaldata` |

## Network

Two architectures, told apart by the version in the RGNN header (`--arch v1|v2`):

```
128 inputs   [our man, our king, their man, their king] x 32 squares, seen from one colour
             (black's view: board rotated 180 degrees, square s -> 31 - s; same as the linear eval)
FT           128 -> FT per point of view, int16 weights (x255), SCReLU: clamp(acc, 0, 255)^2 >> 9   -> 0..127
L1           N -> L1       int8 weights (x64), int32 sums, clamp(sum >> 6, 0, 127)
L2           L1 -> L2      same
output       L2 -> 1, int16 weights (x64)
score        output * out_scale / (127 * 64) centi-men (out_scale 400), clamped to +-14999

v1 (default, the built-in n4c: FT 1024, L1 32, L2 32, 8 buckets)
             N = FT: the side to move's view only. Only the output layer has buckets by piece count.
v2 (--arch v2, default FT 512, L1 32, L2 32, 8 buckets)
             N = 2 FT: the side to move's view followed by the opponent's, through the same FT weights.
             L1, L2 and the output layer each have one set of weights per bucket.
```

Buckets are `min((pieces - 1) * buckets / 24, buckets - 1)`, so `(pieces - 1) / 3` with 8. `--ft` (multiple of
128), `--l1` / `--l2` (16, 32 or 64) and `--buckets` (1..24) set the sizes for either version. The bucketed v2 layers
start with every bucket equal.

The RGNN header records the version, every width and the quantisation constants. The engine reads the widths from it,
so one build loads any v1 or v2 net within its limits: FT up to 1024 per point of view (raise with
`cmake -DREGICIDE_NNUE_MAX_FT=2048`), L1/L2 16/32/64, up to 24 buckets. Anything else is rejected with the reason,
e.g. `cannot load ...: net v2 2x512 -> 48 -> 32 -> 1, 8 buckets (all layers): l1 and l2 must be 16, 32 or 64`.
No build flags are needed for other widths (the old `REGICIDE_NNUE_FT` / `REGICIDE_NNUE_L1` options are gone).

File layout after the 44-byte header (`HB` = 1 in v1, `buckets` in v2; `N` = FT in v1, 2 FT in v2):
`ft_bias i16[FT]`, `ft_weight i16[128][FT]`, `l1_bias i32[HB][L1]`, `l1_weight i8[HB][L1][N]`,
`l2_bias i32[HB][L2]`, `l2_weight i8[HB][L2][L1]`, `out_bias i32[buckets]`, `out_weight i16[buckets][L2]`.
A v1 file is byte-for-byte what earlier versions wrote.

The engine keeps a feature-transformer accumulator per colour's view on its search stack and updates each view
lazily from the moves (moved piece, crowning, captured pieces). A v1 net only needs the side to move's view at a leaf;
a v2 net needs both.

Training target and loss match `tools/train.cpp`. The target is `lambda * result + (1 - lambda) * sigmoid(score / K)`,
with `K` fitted to the data unless `--k` is given. The loss is cross-entropy against `sigmoid(net / K)`. The last
`--valid-frac` (2%) of each file is the validation set. Batches are sampled uniformly at random, with replacement,
from all files (memory-mapped). Features are built on the GPU from the raw bitboards.

Quantisation: after every step, weights are clipped to ranges where the integer net is exact (no int16 accumulator
overflow, int8 hidden weights). The last `--qat-epochs` (2) train through a simulation of the engine's integer
arithmetic: rounded weights and floored activations, with straight-through gradients. Each epoch logs the validation
loss of the float model (`valid`) and of the integer net (`valid(int)`). The exported net is what `valid(int)` measures.

## Setup: Windows + WSL2 + CUDA (RTX 5080)

The RTX 5080 (Blackwell, sm_120) needs PyTorch built for CUDA 12.8 or newer. In WSL (Ubuntu), with the NVIDIA
Windows driver installed (WSL needs no separate Linux driver):

```sh
nvidia-smi                                   # the GPU must be visible inside WSL
sudo apt install -y python3-venv             # once
cd ~/regicide                                # this repository, branch t3/nnue (or later master)
python3 -m venv .venv
.venv/bin/pip install --upgrade pip
.venv/bin/pip install torch --index-url https://download.pytorch.org/whl/cu128
.venv/bin/pip install -r tools/nnue/requirements.txt
.venv/bin/python -c "import torch; print(torch.__version__, torch.cuda.is_available(), torch.cuda.get_device_name(0))"
cmake -S . -B build -G Ninja && cmake --build build && ctest --test-dir build
```

Native Windows Python works the same way (`.venv\Scripts\python`), without `--compile`. Keep the data files on the WSL
filesystem (`~/...`), not on `/mnt/c`, so memory-mapped reads stay fast.

CPU only (smoke tests, the Mac): `python3 -m venv .venv && .venv/bin/pip install -r tools/nnue/requirements.txt`.

## Training run

```sh
# Data: datagen output, as for the linear eval (more positions help the net more than they help the linear eval).
build/datagen --games 400000 --nodes 10000 --threads 22 --seed 31 --out ~/regicide-data/nn1a.bin

# Check the pipeline against the engine first (random net, about 2 seconds).
.venv/bin/python tools/nnue/check_export.py --engine build/regicide --data ~/regicide-data/nn1a.bin

# Train: ~100M positions per epoch, 40 epochs (about 4B samples).
.venv/bin/python tools/nnue/train.py \
    --data ~/regicide-data/gen1_n10k.bin --data ~/regicide-data/gen1b_n10k.bin --data ~/regicide-data/gen1c_n10k.bin \
    --data ~/regicide-data/nn1a.bin \
    --out runs/nn1 --epochs 40 --epoch-size 100000000 --batch 16384 \
    --lr 1e-3 --lr-final 1e-5 --wd 0.01 --lambda 0.7 --qat-epochs 2 --compile \
    2>&1 | tee runs/nn1.log

# A v2 net: add --arch v2 and the sizes (defaults FT 512, L1 32, L2 32, 8 buckets), e.g.
#   --arch v2 --ft 512 --buckets 8          (as fast as v1 FT 1024)
#   --arch v2 --ft 1024 --l1 16             (about 78% of that speed on the M5)

# Resume after an interruption (same arguments, plus --resume):
.venv/bin/python tools/nnue/train.py <same arguments> --resume runs/nn1/last.pt

# Verify the final net against the engine on 1000 validation-file positions, then try it. Without --checkpoint,
# check_export tests a random net of --arch / --ft / --l1 / --l2 / --buckets.
.venv/bin/python tools/nnue/check_export.py --engine build/regicide --data ~/regicide-data/nn1a.bin \
    --checkpoint runs/nn1/last.pt
printf 'set evalfile runs/nn1/net_e040.nnue\nbench 15\nquit\n' | build/regicide
```

Use the paths of your gen1/gen2 files for `--data`. The gen1 files have about 69M positions, with K around 99.
Each epoch writes `runs/nn1/net_eNNN.nnue`, `ckpt_eNNN.pt` and `last.pt`. `config.json` records the arguments and
the fitted K.

## Using a net

- Engine: `set evalfile path/to/net.nnue` (or `set evalfile builtin`), and in CheckerBoard / the DLL,
  `enginecommand("set evalfile <path>")`. The header tells RGEV and RGNN files apart.
- Tools: `selfplay --evala net.nnue` / `--evalb`, `datagen --evalfile net.nnue` take either format.
- Built in: copy the net to `src/weights/net.bin` and rebuild. It then replaces the linear eval.
  `src/weights/net.bin` is empty by default, and the linear eval is used. `ctest` checks that an embedded net loads.
- `regicide evaldata <net> <data.bin> [n]` prints the engine's eval of each record (used by `check_export.py`).
