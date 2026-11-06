#pragma once
// WLD compression: byte-oriented run-length tokens in independently decodable groups.
//
// Positions are split into groups of 1024 consecutive indices; 32 groups form a superblock.
// Random access: superblock byte offset (u32) + group offset within the superblock (u16), then a
// short forward scan over the group's tokens. Each token covers >= 4 positions:
//   0..80    four positions, base-3 digits (value of position i is digit i)
//   81..254  a run: value (t - 81) / 58, length RUNLEN[(t - 81) % 58]   (4 .. 1024)
// Values: 0 = win, 1 = draw, 2 = loss (side to move's view). The encoder also takes "don't care"
// inputs (capture positions, unused index entries) and assigns them whatever extends runs.

#include <array>
#include <cstdint>
#include <vector>

namespace regicide::egdb::detail::codec {

inline constexpr int GROUP_SHIFT = 10;
inline constexpr std::uint32_t GROUP = 1u << GROUP_SHIFT;
inline constexpr int SUPER_SHIFT = 5;  // groups per superblock = 32 (keeps u16 group offsets safe for MTC)
inline constexpr int NRUN = 58;

enum : std::uint8_t { V_WIN = 0, V_DRAW = 1, V_LOSS = 2, V_DC = 3 };

struct Tables {
    std::uint16_t runlen[NRUN];
    std::uint16_t len[256];
    std::uint8_t val[256];
    std::uint8_t quad[81][4];
};

constexpr Tables make_tables() {
    Tables t{};
    // Roughly geometric run lengths 4..1024 (ratio ~1.095); greedy sums express any length.
    std::uint32_t l = 4;
    for (int k = 0; k < NRUN; ++k) {
        t.runlen[k] = std::uint16_t(l);
        std::uint32_t n = (l * 1095 + 500) / 1000;
        l = n > l ? n : l + 1;
    }
    t.runlen[NRUN - 1] = GROUP;
    for (int i = 0; i < 81; ++i) {
        t.len[i] = 4;
        t.val[i] = 0;
        for (int j = 0, v = i; j < 4; ++j, v /= 3) t.quad[i][j] = std::uint8_t(v % 3);
    }
    for (int i = 81; i < 256; ++i) {
        const int r = i - 81;
        t.len[i] = r < 3 * NRUN ? t.runlen[r % NRUN] : 0xFFFF;
        t.val[i] = std::uint8_t(r / NRUN);
    }
    return t;
}
inline constexpr Tables TT = make_tables();
static_assert(TT.runlen[NRUN - 2] < TT.runlen[NRUN - 1]);

inline int decode(const std::uint32_t* sup, const std::uint16_t* grp, const std::uint8_t* data, std::uint64_t idx) {
    const std::uint64_t g = idx >> GROUP_SHIFT;
    const std::uint8_t* p = data + sup[g >> SUPER_SHIFT] + grp[g];
    std::uint32_t rem = std::uint32_t(idx) & (GROUP - 1);
    for (;;) {
        const std::uint8_t t = *p++;
        const std::uint32_t len = TT.len[t];
        if (rem < len) return t < 81 ? TT.quad[t][rem] : TT.val[t];
        rem -= len;
    }
}

struct Encoded {
    enum Kind : std::uint8_t { Constant = 1, Packed = 2 };
    Kind kind = Constant;
    std::uint8_t value = V_DRAW;  // for Constant
    std::uint64_t size = 0;       // positions
    std::vector<std::uint32_t> sup;
    std::vector<std::uint16_t> grp;
    std::vector<std::uint8_t> data;
};

// Encode values (V_WIN/V_DRAW/V_LOSS/V_DC) of n positions. `threads` workers encode superblocks.
Encoded encode(const std::uint8_t* vals, std::uint64_t n, int threads);

// Moves-to-conversion tables use the same group/superblock layout with a different token stream.
// Each group starts with last = 0. Tokens:
//   0..223   literal value (one position), becomes `last`
//   224..254 run of MRUN[t - 224] positions whose value is `last`
//   255      escape: the next byte is a literal value (224..255; 255 means >= 255)
// Inputs are 0..65534, or MTC_DC for positions whose value is irrelevant (draws, captures, unused).
namespace mtc {
inline constexpr std::uint16_t MTC_DC = 0xFFFF;
inline constexpr std::uint16_t MRUN[31] = {1,  2,  3,  4,  5,  6,   7,   8,   10,  12,  14,  16,  20,  24,  28, 32,
                                           40, 48, 56, 64, 80, 96, 128, 160, 192, 256, 320, 384, 512, 768, 1024};

inline int decode(const std::uint32_t* sup, const std::uint16_t* grp, const std::uint8_t* data, std::uint64_t idx) {
    const std::uint64_t g = idx >> GROUP_SHIFT;
    const std::uint8_t* p = data + sup[g >> SUPER_SHIFT] + grp[g];
    std::uint32_t rem = std::uint32_t(idx) & (GROUP - 1);
    int last = 0;
    for (;;) {
        const std::uint8_t t = *p++;
        if (t < 224 || t == 255) {
            const int v = t == 255 ? *p++ : t;
            if (rem == 0) return v;
            last = v;
            --rem;
        } else {
            const std::uint32_t len = MRUN[t - 224];
            if (rem < len) return last;
            rem -= len;
        }
    }
}

Encoded encode(const std::uint16_t* vals, std::uint64_t n, int threads);
}  // namespace mtc

}  // namespace regicide::egdb::detail::codec
