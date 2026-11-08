"""Shared NNUE code: architecture, training data, features, model, quantisation and the RGNN format.

Everything here must match src/nnue.hpp / src/nnue.cpp:
  inputs   128 = [our man, our king, their man, their king] x 32 squares, seen from the side to move, which
           always moves "up": positions with black to move are rotated 180 degrees (square s -> 31 - s).
  FT       128 -> 256, SCReLU  a = clamp(acc, 0, 1)^2      int: acc in 1/255, a = clamp(acc, 0, 255)^2 >> 9
  L1       256 -> 32,  CReLU   h = clamp(x, 0, 1)          int: weights in 1/64, h = clamp(sum >> 6, 0, 127)
  L2       32 -> 32,   CReLU
  output   32 -> 1, one of 8 buckets by piece count;       score (centi-men) = output * out_scale
"""

import struct
import threading
import queue

import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F

INPUTS, FT, L1, L2, BUCKETS = 128, 768, 32, 32, 8
ACTIVATION, QA, QB, QH = 1, 255, 64, 127
VERSION = 1
OUT_SCALE = 400
SCORE_LIMIT = 14999  # the engine clamps static evals to +-(SCORE_KNOWN_WIN - 1)

# Ranges that keep the integer net exact: the int16 accumulator (bias + up to 24 weights, in 1/255) must not
# overflow, and hidden weights must fit int8 in 1/64.
FT_WEIGHT_MAX = 5.0
FT_BIAS_MAX = 8.0
HIDDEN_WEIGHT_MAX = 127 / QB
OUT_WEIGHT_MAX = 500.0

# One training position (src/datafile.hpp), 16 bytes little endian.
RECORD = np.dtype([("black", "<u4"), ("white", "<u4"), ("kings", "<u4"), ("score", "<i2"), ("stm", "u1"),
                   ("result", "i1")])
BLACK, WHITE = 0, 1


def bucket_of(pieces):
    """Output bucket for a piece count (works on ints, numpy arrays and torch tensors)."""
    return ((pieces - 1).clip(0) * BUCKETS // 24).clip(max=BUCKETS - 1) if not isinstance(pieces, int) else \
        min(max(pieces - 1, 0) * BUCKETS // 24, BUCKETS - 1)


def stm_result(rec):
    """Game result from the side to move's point of view, 0 / 0.5 / 1."""
    r = rec["result"].astype(np.float64)
    return 0.5 + 0.5 * np.where(rec["stm"] == BLACK, r, -r)


# ----------------------------------------------------------------------------------------------------------------
# Data


class Data:
    """Several DataRecord files, memory-mapped. The last `valid_frac` of each file is held out for validation
    (files are written game by game, so this keeps validation games apart from training games)."""

    def __init__(self, files, valid_frac=0.02):
        self.maps, self.train_sizes, self.valid_parts = [], [], []
        for f in files:
            m = np.memmap(f, dtype=RECORD, mode="r")
            nv = int(len(m) * valid_frac)
            self.maps.append(m)
            self.train_sizes.append(len(m) - nv)
            self.valid_parts.append(m[len(m) - nv:])
        self.train_ends = np.cumsum(self.train_sizes)
        self.n_train = int(self.train_ends[-1]) if files else 0
        self.n_valid = sum(len(v) for v in self.valid_parts)

    def gather(self, idx):
        """Training records at global indices `idx` (sorted for locality)."""
        idx = np.sort(idx)
        which = np.searchsorted(self.train_ends, idx, side="right")
        starts = np.concatenate([[0], self.train_ends[:-1]])
        out = np.empty(len(idx), dtype=RECORD)
        for f in np.unique(which):
            sel = which == f
            out[sel] = self.maps[f][idx[sel] - starts[f]]
        return out

    def sample(self, rng, n):
        """Uniform random training records (with replacement)."""
        return self.gather(rng.integers(0, self.n_train, n))

    def validation(self, limit, rng):
        v = np.concatenate([np.asarray(p) for p in self.valid_parts]) if self.valid_parts else np.empty(0, RECORD)
        if len(v) > limit:
            v = v[np.sort(rng.choice(len(v), limit, replace=False))]
        return v


class Prefetcher:
    """Background threads that sample batches while the model trains."""

    def __init__(self, data, batch, seed, depth=8, workers=2):
        self.q = queue.Queue(maxsize=depth)
        self.stop = False
        self.threads = []
        for w in range(workers):
            rng = np.random.default_rng([seed, w])
            t = threading.Thread(target=self._run, args=(data, batch, rng), daemon=True)
            t.start()
            self.threads.append(t)

    def _run(self, data, batch, rng):
        while not self.stop:
            self.q.put(data.sample(rng, batch))

    def next(self):
        return self.q.get()

    def close(self):
        self.stop = True
        while any(t.is_alive() for t in self.threads):
            try:
                self.q.get_nowait()
            except queue.Empty:
                pass
            for t in self.threads:
                t.join(timeout=0.01)


def to_tensors(rec, device):
    """Raw record fields as int64 tensors on `device`."""
    cols = {k: torch.from_numpy(rec[k].astype(np.int64)) for k in ("black", "white", "kings", "stm", "score")}
    cols["result"] = torch.from_numpy(stm_result(rec).astype(np.float32))
    pin = torch.device(device).type == "cuda"
    return {k: (v.pin_memory() if pin else v).to(device, non_blocking=True) for k, v in cols.items()}


_SHIFTS = {}


def features(black, white, kings, stm):
    """Dense 0/1 inputs [B, 128] and output buckets [B] from int64 bitboard tensors."""
    dev = black.device
    if dev not in _SHIFTS:
        _SHIFTS[dev] = torch.arange(32, device=dev, dtype=torch.int64)
    us = torch.where(stm == BLACK, black, white)
    them = torch.where(stm == BLACK, white, black)
    planes = torch.stack([us & ~kings, us & kings, them & ~kings, them & kings], 1)  # [B, 4]
    bits = (planes.unsqueeze(-1) >> _SHIFTS[dev]) & 1  # [B, 4, 32], bit s = square s
    bits = torch.where((stm == BLACK)[:, None, None], bits.flip(-1), bits)  # black's view: s -> 31 - s
    x = bits.reshape(-1, INPUTS).float()
    pieces = bits.sum((1, 2))
    return x, bucket_of(pieces)


def features_np(rec):
    """numpy version of features(): int64 inputs [B, 128] and buckets [B]."""
    black, white, kings = (rec[k].astype(np.int64) for k in ("black", "white", "kings"))
    stm = rec["stm"].astype(np.int64)
    us = np.where(stm == BLACK, black, white)
    them = np.where(stm == BLACK, white, black)
    planes = np.stack([us & ~kings, us & kings, them & ~kings, them & kings], 1)
    bits = (planes[:, :, None] >> np.arange(32)) & 1
    bits = np.where((stm == BLACK)[:, None, None], bits[:, :, ::-1], bits)
    pieces = bits.sum((1, 2))
    return bits.reshape(-1, INPUTS), bucket_of(pieces)


def fit_k(rec, lo=20.0, hi=2000.0):
    """K such that sigmoid(score / K) best predicts the game results (as tools/train.cpp)."""
    s = rec["score"].astype(np.float64)
    t = stm_result(rec)

    def loss(k):
        p = np.clip(1 / (1 + np.exp(-s / k)), 1e-9, 1 - 1e-9)
        return -np.mean(t * np.log(p) + (1 - t) * np.log(1 - p))

    for _ in range(60):
        m1, m2 = lo + (hi - lo) / 3, hi - (hi - lo) / 3
        if loss(m1) < loss(m2):
            hi = m2
        else:
            lo = m1
    return (lo + hi) / 2


# ----------------------------------------------------------------------------------------------------------------
# Model


def ste(x, q):
    """Straight-through estimator: forward exactly q, gradient of x."""
    return q.detach() + (x - x.detach())


class Net(nn.Module):
    def __init__(self, out_scale=OUT_SCALE):
        super().__init__()
        self.out_scale = out_scale
        self.ft = nn.Linear(INPUTS, FT)
        self.l1 = nn.Linear(FT, L1)
        self.l2 = nn.Linear(L1, L2)
        self.out = nn.Linear(L2, BUCKETS)
        with torch.no_grad():
            # Positive FT biases start most SCReLU units in their active range; small output start.
            self.ft.bias.uniform_(0.0, 0.5)
            self.out.weight.mul_(0.5)
            self.out.bias.zero_()

    def forward(self, x, bucket, quant=False):
        """Score in centi-men, side to move's view. quant=True simulates the engine's integer arithmetic
        (rounded weights, floored activations) with straight-through gradients."""
        if not quant:
            a = self.ft(x).clamp(0, 1).pow(2)
            h1 = self.l1(a).clamp(0, 1)
            h2 = self.l2(h1).clamp(0, 1)
            y = self.out(h2).gather(1, bucket.view(-1, 1)).squeeze(1)
            return y * self.out_scale
        acc = F.linear(x, ste(self.ft.weight * QA, torch.round(self.ft.weight * QA)),
                       ste(self.ft.bias * QA, torch.round(self.ft.bias * QA)))
        c = acc.clamp(0, QA)
        a = ste(c * c / 512, torch.floor(c * c / 512))  # 0..127

        def hidden(layer, inp):
            w = ste(layer.weight * QB, torch.round(layer.weight * QB))
            b = ste(layer.bias * QH * QB, torch.round(layer.bias * QH * QB))
            s = F.linear(inp, w, b) / QB
            return ste(s, torch.floor(s)).clamp(0, QH)

        h1 = hidden(self.l1, a)
        h2 = hidden(self.l2, h1)
        w = ste(self.out.weight * QB, torch.round(self.out.weight * QB))
        b = ste(self.out.bias * QH * QB, torch.round(self.out.bias * QH * QB))
        y = F.linear(h2, w, b).gather(1, bucket.view(-1, 1)).squeeze(1)
        return y * self.out_scale / (QH * QB)

    @torch.no_grad()
    def clip_(self):
        """Keep weights where the integer net is exact (called after every optimiser step)."""
        self.ft.weight.clamp_(-FT_WEIGHT_MAX, FT_WEIGHT_MAX)
        self.ft.bias.clamp_(-FT_BIAS_MAX, FT_BIAS_MAX)
        for layer in (self.l1, self.l2):
            layer.weight.clamp_(-HIDDEN_WEIGHT_MAX, HIDDEN_WEIGHT_MAX)
        self.out.weight.clamp_(-OUT_WEIGHT_MAX, OUT_WEIGHT_MAX)


# ----------------------------------------------------------------------------------------------------------------
# Quantisation and the RGNN file


def quantise(model):
    """Integer weights as the engine stores them (numpy arrays, file order and shapes)."""
    def q(t, scale, dtype, lo, hi):
        # Rounded in float32 exactly as Net.forward(quant=True) does, so training simulates this net.
        return np.clip(torch.round(t.detach().float().cpu() * scale).double().numpy(), lo, hi).astype(dtype)

    i16, i32 = (-32767, 32767), (-(2 ** 31) + 1, 2 ** 31 - 1)
    return {
        "ft_bias": q(model.ft.bias, QA, np.int16, *i16),
        "ft_weight": q(model.ft.weight.t(), QA, np.int16, *i16),  # [INPUTS][FT]
        "l1_bias": q(model.l1.bias, QH * QB, np.int32, *i32),
        "l1_weight": q(model.l1.weight, QB, np.int8, -127, 127),  # [L1][FT]
        "l2_bias": q(model.l2.bias, QH * QB, np.int32, *i32),
        "l2_weight": q(model.l2.weight, QB, np.int8, -127, 127),  # [L2][L1]
        "out_bias": q(model.out.bias, QH * QB, np.int32, *i32),
        "out_weight": q(model.out.weight, QB, np.int16, *i16),  # [BUCKETS][L2]
        "out_scale": int(model.out_scale),
    }


ORDER = ["ft_bias", "ft_weight", "l1_bias", "l1_weight", "l2_bias", "l2_weight", "out_bias", "out_weight"]


def write_rgnn(path, qw):
    header = b"RGNN" + struct.pack("<10I", VERSION, INPUTS, FT, L1, L2, BUCKETS, ACTIVATION, QA, QB,
                                   qw["out_scale"])
    with open(path, "wb") as f:
        f.write(header)
        for k in ORDER:
            f.write(np.ascontiguousarray(qw[k]).astype(qw[k].dtype.newbyteorder("<")).tobytes())


def read_rgnn(path):
    data = open(path, "rb").read()
    if data[:4] != b"RGNN":
        raise ValueError("not an RGNN file")
    h = struct.unpack("<10I", data[4:44])
    if h[:9] != (VERSION, INPUTS, FT, L1, L2, BUCKETS, ACTIVATION, QA, QB):
        raise ValueError("architecture mismatch: %s" % (h,))
    shapes = {"ft_bias": (FT,), "ft_weight": (INPUTS, FT), "l1_bias": (L1,), "l1_weight": (L1, FT),
              "l2_bias": (L2,), "l2_weight": (L2, L1), "out_bias": (BUCKETS,), "out_weight": (BUCKETS, L2)}
    dtypes = {"ft_bias": "<i2", "ft_weight": "<i2", "l1_bias": "<i4", "l1_weight": "i1", "l2_bias": "<i4",
              "l2_weight": "i1", "out_bias": "<i4", "out_weight": "<i2"}
    qw, off = {"out_scale": h[9]}, 44
    for k in ORDER:
        n = int(np.prod(shapes[k]))
        qw[k] = np.frombuffer(data, dtype=dtypes[k], count=n, offset=off).reshape(shapes[k])
        off += n * np.dtype(dtypes[k]).itemsize
    if off != len(data):
        raise ValueError("RGNN size %d, expected %d" % (len(data), off))
    return qw


def forward_int(qw, rec):
    """The engine's integer forward pass on records (numpy int64; must match Network::evaluate exactly)."""
    x, b = features_np(rec)
    acc = qw["ft_bias"].astype(np.int64) + x @ qw["ft_weight"].astype(np.int64)
    assert np.all(np.abs(acc) <= 32767), "int16 accumulator overflow"
    c = np.clip(acc, 0, QA)
    a = (c * c) >> 9
    h1 = np.clip((a @ qw["l1_weight"].astype(np.int64).T + qw["l1_bias"]) >> 6, 0, QH)
    h2 = np.clip((h1 @ qw["l2_weight"].astype(np.int64).T + qw["l2_bias"]) >> 6, 0, QH)
    out = np.einsum("bj,bj->b", h2, qw["out_weight"].astype(np.int64)[b]) + qw["out_bias"].astype(np.int64)[b]
    prod = out * qw["out_scale"]
    s = np.sign(prod) * (np.abs(prod) // (QH * QB))  # C++ division truncates toward zero
    return np.clip(s, -SCORE_LIMIT, SCORE_LIMIT)
