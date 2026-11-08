"""Shared NNUE code: architectures, training data, features, model, quantisation and the RGNN format.

Everything here must match src/nnue.hpp / src/nnue.cpp:
  inputs   128 = [our man, our king, their man, their king] x 32 squares, seen from one colour, which always
           moves "up": black's view is rotated 180 degrees (square s -> 31 - s).
  FT       128 -> FT, SCReLU  a = clamp(acc, 0, 1)^2       int: acc in 1/255, a = clamp(acc, 0, 255)^2 >> 9
  L1       N -> L1,   CReLU   h = clamp(x, 0, 1)           int: weights in 1/64, h = clamp(sum >> 6, 0, 127)
  L2       L1 -> L2,  CReLU
  output   L2 -> 1                                          score (centi-men) = output * out_scale

  v1: N = FT, the side to move's view; only the output layer has buckets (by piece count).
  v2: N = 2 FT, the side to move's view followed by the opponent's (shared FT weights); L1, L2 and the output
      layer all have one set of weights per bucket.
"""

import collections
import struct
import threading
import queue

import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F

INPUTS = 128
ACTIVATION, QA, QB, QH = 1, 255, 64, 127
OUT_SCALE = 400
SCORE_LIMIT = 14999  # the engine clamps static evals to +-(SCORE_KNOWN_WIN - 1)
HIDDEN_WIDTHS = (16, 32, 64)  # L1 / L2 widths the engine has kernels for
FT_TILE = 128  # FT widths are multiples of this
MAX_BUCKETS = 24


class Arch(collections.namedtuple("Arch", "version ft l1 l2 buckets")):
    """Layer sizes of a net (the RGNN header). ft is per point of view."""

    @property
    def views(self):
        return 2 if self.version == 2 else 1

    @property
    def l1_inputs(self):
        return self.views * self.ft

    @property
    def hidden_buckets(self):
        return self.buckets if self.version == 2 else 1

    def check(self):
        if self.version not in (1, 2):
            raise ValueError("arch version must be 1 or 2")
        if self.ft <= 0 or self.ft % FT_TILE:
            raise ValueError("ft must be a positive multiple of %d" % FT_TILE)
        if self.l1 not in HIDDEN_WIDTHS or self.l2 not in HIDDEN_WIDTHS:
            raise ValueError("l1 and l2 must be one of %s" % (HIDDEN_WIDTHS,))
        if not 1 <= self.buckets <= MAX_BUCKETS:
            raise ValueError("buckets must be 1..%d" % MAX_BUCKETS)
        return self

    def __str__(self):
        return "v%d %s%d -> %d -> %d -> 1, %d buckets" % (self.version, "2x" if self.views == 2 else "", self.ft,
                                                         self.l1, self.l2, self.buckets)


V1 = Arch(1, 1024, 32, 32, 8)  # the built-in net's architecture (n4c)
V2 = Arch(2, 512, 32, 32, 8)   # default sizes for --arch v2
DEFAULTS = {"v1": V1, "v2": V2}


def make_arch(name="v1", ft=None, l1=None, l2=None, buckets=None):
    """Arch for --arch NAME with any sizes overridden."""
    d = DEFAULTS[name]
    return Arch(d.version, ft or d.ft, l1 or d.l1, l2 or d.l2, buckets or d.buckets).check()


def arch_of_state(state):
    """The Arch of a model state_dict (works for checkpoints from before the arch was recorded)."""
    ft = state["ft.weight"].shape[0]
    n1 = state["l1.weight"].shape[1]
    buckets = state["out.weight"].shape[0]
    version = 2 if n1 == 2 * ft else 1
    hb = buckets if version == 2 else 1
    return Arch(version, ft, state["l1.weight"].shape[0] // hb, state["l2.weight"].shape[0] // hb, buckets)


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


def bucket_of(pieces, buckets):
    """Bucket for a piece count (works on ints, numpy arrays and torch tensors)."""
    return ((pieces - 1).clip(0) * buckets // 24).clip(max=buckets - 1) if not isinstance(pieces, int) else \
        min(max(pieces - 1, 0) * buckets // 24, buckets - 1)


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


def features(black, white, kings, view):
    """Dense 0/1 inputs [B, 128] seen from `view` (int64 colour tensor), and the piece counts [B], from int64
    bitboard tensors."""
    dev = black.device
    if dev not in _SHIFTS:
        _SHIFTS[dev] = torch.arange(32, device=dev, dtype=torch.int64)
    us = torch.where(view == BLACK, black, white)
    them = torch.where(view == BLACK, white, black)
    planes = torch.stack([us & ~kings, us & kings, them & ~kings, them & kings], 1)  # [B, 4]
    bits = (planes.unsqueeze(-1) >> _SHIFTS[dev]) & 1  # [B, 4, 32], bit s = square s
    bits = torch.where((view == BLACK)[:, None, None], bits.flip(-1), bits)  # black's view: s -> 31 - s
    return bits.reshape(-1, INPUTS).float(), bits.sum((1, 2))


def inputs(arch, black, white, kings, stm):
    """The model's inputs: (side to move's view, opponent's view or None for v1, bucket), from int64 tensors."""
    x, pieces = features(black, white, kings, stm)
    xo = features(black, white, kings, 1 - stm)[0] if arch.views == 2 else None
    return x, xo, bucket_of(pieces, arch.buckets)


def features_np(rec, view):
    """numpy version of features(): int64 inputs [B, 128] seen from `view` (array of colours) and piece counts."""
    black, white, kings = (rec[k].astype(np.int64) for k in ("black", "white", "kings"))
    us = np.where(view == BLACK, black, white)
    them = np.where(view == BLACK, white, black)
    planes = np.stack([us & ~kings, us & kings, them & ~kings, them & kings], 1)
    bits = (planes[:, :, None] >> np.arange(32)) & 1
    bits = np.where((view == BLACK)[:, None, None], bits[:, :, ::-1], bits)
    return bits.reshape(-1, INPUTS), bits.sum((1, 2))


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


def select(y, bucket, n):
    """Rows of y [B, buckets * n] narrowed to each row's bucket: [B, n]."""
    if y.shape[1] == n:
        return y
    return y.view(len(y), -1, n).gather(1, bucket.view(-1, 1, 1).expand(-1, 1, n)).squeeze(1)


class Net(nn.Module):
    def __init__(self, arch=V1, out_scale=OUT_SCALE):
        super().__init__()
        self.arch = arch
        self.out_scale = out_scale
        hb = arch.hidden_buckets
        self.ft = nn.Linear(INPUTS, arch.ft)
        self.l1 = nn.Linear(arch.l1_inputs, hb * arch.l1)  # rows: bucket by bucket
        self.l2 = nn.Linear(arch.l1, hb * arch.l2)
        self.out = nn.Linear(arch.l2, arch.buckets)
        with torch.no_grad():
            # Positive FT biases start most SCReLU units in their active range; small output start.
            self.ft.bias.uniform_(0.0, 0.5)
            self.out.weight.mul_(0.5)
            self.out.bias.zero_()
            # Bucketed hidden layers start with every bucket equal.
            for layer, n in ((self.l1, arch.l1), (self.l2, arch.l2)):
                layer.weight.copy_(layer.weight[:n].repeat(hb, 1))
                layer.bias.copy_(layer.bias[:n].repeat(hb))

    def forward(self, x, xo, bucket, quant=False):
        """Score in centi-men, side to move's view, from inputs() (xo: the opponent's view, v2 only).
        quant=True simulates the engine's integer arithmetic (rounded weights, floored activations) with
        straight-through gradients."""
        a = self.arch
        if not quant:
            act = self.ft(x).clamp(0, 1).pow(2)
            if xo is not None:
                act = torch.cat([act, self.ft(xo).clamp(0, 1).pow(2)], 1)
            h1 = select(self.l1(act), bucket, a.l1).clamp(0, 1)
            h2 = select(self.l2(h1), bucket, a.l2).clamp(0, 1)
            y = self.out(h2).gather(1, bucket.view(-1, 1)).squeeze(1)
            return y * self.out_scale
        ftw = ste(self.ft.weight * QA, torch.round(self.ft.weight * QA))
        ftb = ste(self.ft.bias * QA, torch.round(self.ft.bias * QA))

        def transform(inp):
            c = F.linear(inp, ftw, ftb).clamp(0, QA)
            return ste(c * c / 512, torch.floor(c * c / 512))  # 0..127

        act = transform(x) if xo is None else torch.cat([transform(x), transform(xo)], 1)

        def hidden(layer, inp, n):
            w = ste(layer.weight * QB, torch.round(layer.weight * QB))
            b = ste(layer.bias * QH * QB, torch.round(layer.bias * QH * QB))
            s = select(F.linear(inp, w, b), bucket, n) / QB
            return ste(s, torch.floor(s)).clamp(0, QH)

        h1 = hidden(self.l1, act, a.l1)
        h2 = hidden(self.l2, h1, a.l2)
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


def load_model(ck):
    """The model of a training checkpoint (train.py's dict), with the arch its weights have."""
    model = Net(arch_of_state(ck["model"]), ck["args"].get("out_scale", OUT_SCALE))
    model.load_state_dict(ck["model"])
    return model


# ----------------------------------------------------------------------------------------------------------------
# Quantisation and the RGNN file


def quantise(model):
    """Integer weights as the engine stores them (numpy arrays, file order; hidden layers bucket by bucket)."""
    def q(t, scale, dtype, lo, hi):
        # Rounded in float32 exactly as Net.forward(quant=True) does, so training simulates this net.
        return np.clip(torch.round(t.detach().float().cpu() * scale).double().numpy(), lo, hi).astype(dtype)

    i16, i32 = (-32767, 32767), (-(2 ** 31) + 1, 2 ** 31 - 1)
    return {
        "arch": model.arch,
        "ft_bias": q(model.ft.bias, QA, np.int16, *i16),
        "ft_weight": q(model.ft.weight.t(), QA, np.int16, *i16),  # [INPUTS][FT]
        "l1_bias": q(model.l1.bias, QH * QB, np.int32, *i32),  # [HB * L1]
        "l1_weight": q(model.l1.weight, QB, np.int8, -127, 127),  # [HB * L1][N]
        "l2_bias": q(model.l2.bias, QH * QB, np.int32, *i32),  # [HB * L2]
        "l2_weight": q(model.l2.weight, QB, np.int8, -127, 127),  # [HB * L2][L1]
        "out_bias": q(model.out.bias, QH * QB, np.int32, *i32),  # [BUCKETS]
        "out_weight": q(model.out.weight, QB, np.int16, *i16),  # [BUCKETS][L2]
        "out_scale": int(model.out_scale),
    }


ORDER = ["ft_bias", "ft_weight", "l1_bias", "l1_weight", "l2_bias", "l2_weight", "out_bias", "out_weight"]
DTYPES = {"ft_bias": "<i2", "ft_weight": "<i2", "l1_bias": "<i4", "l1_weight": "i1", "l2_bias": "<i4",
          "l2_weight": "i1", "out_bias": "<i4", "out_weight": "<i2"}


def shapes(arch):
    hb = arch.hidden_buckets
    return {"ft_bias": (arch.ft,), "ft_weight": (INPUTS, arch.ft), "l1_bias": (hb * arch.l1,),
            "l1_weight": (hb * arch.l1, arch.l1_inputs), "l2_bias": (hb * arch.l2,),
            "l2_weight": (hb * arch.l2, arch.l1), "out_bias": (arch.buckets,), "out_weight": (arch.buckets, arch.l2)}


def write_rgnn(path, qw):
    a = qw["arch"]
    header = b"RGNN" + struct.pack("<10I", a.version, INPUTS, a.ft, a.l1, a.l2, a.buckets, ACTIVATION, QA, QB,
                                   qw["out_scale"])
    with open(path, "wb") as f:
        f.write(header)
        for k, shape in shapes(a).items():
            assert qw[k].shape == shape, (k, qw[k].shape, shape)
            f.write(np.ascontiguousarray(qw[k]).astype(DTYPES[k]).tobytes())


def read_rgnn(path):
    data = open(path, "rb").read()
    if data[:4] != b"RGNN":
        raise ValueError("not an RGNN file")
    h = struct.unpack("<10I", data[4:44])
    if h[1] != INPUTS or h[6:9] != (ACTIVATION, QA, QB):
        raise ValueError("unsupported RGNN header: %s" % (h,))
    arch = Arch(*h[0:1], *h[2:6]).check()
    qw, off = {"arch": arch, "out_scale": h[9]}, 44
    for k, shape in shapes(arch).items():
        n = int(np.prod(shape))
        qw[k] = np.frombuffer(data, dtype=DTYPES[k], count=n, offset=off).reshape(shape)
        off += n * np.dtype(DTYPES[k]).itemsize
    if off != len(data):
        raise ValueError("RGNN size %d, expected %d" % (len(data), off))
    return qw


def forward_int(qw, rec):
    """The engine's integer forward pass on records (numpy int64; must match Network::evaluate exactly)."""
    a = qw["arch"]
    stm = rec["stm"].astype(np.int64)

    def transform(view):
        x, pieces = features_np(rec, view)
        acc = qw["ft_bias"].astype(np.int64) + x @ qw["ft_weight"].astype(np.int64)
        assert np.all(np.abs(acc) <= 32767), "int16 accumulator overflow"
        c = np.clip(acc, 0, QA)
        return (c * c) >> 9, pieces

    act, pieces = transform(stm)
    if a.views == 2:
        act = np.concatenate([act, transform(1 - stm)[0]], 1)
    b = bucket_of(pieces, a.buckets)
    hb = b if a.version == 2 else np.zeros_like(b)
    rows = np.arange(len(rec))

    def hidden(inp, weight, bias, n):
        s = inp @ qw[weight].astype(np.int64).T + qw[bias].astype(np.int64)  # [B, HB * n]
        s = s.reshape(len(rec), -1, n)[rows, hb]
        return np.clip(s >> 6, 0, QH)

    h1 = hidden(act, "l1_weight", "l1_bias", a.l1)
    h2 = hidden(h1, "l2_weight", "l2_bias", a.l2)
    out = np.einsum("bj,bj->b", h2, qw["out_weight"].astype(np.int64)[b]) + qw["out_bias"].astype(np.int64)[b]
    prod = out * qw["out_scale"]
    s = np.sign(prod) * (np.abs(prod) // (QH * QB))  # C++ division truncates toward zero
    return np.clip(s, -SCORE_LIMIT, SCORE_LIMIT)
