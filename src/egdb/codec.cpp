#include "egdb/codec.hpp"

#include <algorithm>
#include <stdexcept>

#include "egdb/parallel.hpp"

namespace regicide::egdb::detail::codec {

namespace {

// Encodes positions [gs, ge) of one group, appending tokens to out.
void encode_group(const std::uint8_t* v, std::uint64_t gs, std::uint64_t ge, std::vector<std::uint8_t>& out) {
    {
        std::uint8_t last = V_DRAW;
        for (std::uint64_t p = gs; p < ge;) {
            // Longest run starting at p, treating don't-cares as wildcards.
            std::uint64_t q = p;
            unsigned alive = 7;  // bit v: value v still matches
            int best = -1;
            while (q < ge) {
                const std::uint8_t x = v[q];
                const unsigned nalive = x == V_DC ? alive : alive & (1u << x);
                if (!nalive) break;
                alive = nalive;
                ++q;
            }
            const std::uint64_t run = q - p;
            best = (alive & (1u << last)) ? last : std::countr_zero(alive);
            if (run >= 4) {
                int k = NRUN - 1;
                while (TT.runlen[k] > run) --k;
                out.push_back(std::uint8_t(81 + best * NRUN + k));
                p += TT.runlen[k];
                last = std::uint8_t(best);
            } else {
                int t = 0;
                for (int i = 3; i >= 0; --i) {
                    std::uint8_t x = p + i < ge ? v[p + i] : V_DC;
                    if (x == V_DC) x = best;
                    t = t * 3 + x;
                }
                out.push_back(std::uint8_t(t));
                std::uint8_t tail = p + 3 < ge ? v[p + 3] : V_DC;
                last = tail == V_DC ? std::uint8_t(best) : tail;
                p += 4;
            }
        }
    }
}

template <class GroupFn>
Encoded encode_groups(std::uint64_t n, int threads, GroupFn&& fn) {
    Encoded e;
    e.size = n;
    e.kind = Encoded::Packed;
    const std::uint64_t ngroups = (n + GROUP - 1) / GROUP;
    const std::uint64_t nsuper = (ngroups + (1u << SUPER_SHIFT) - 1) >> SUPER_SHIFT;
    const std::uint64_t super_pos = std::uint64_t(GROUP) << SUPER_SHIFT;
    std::vector<std::vector<std::uint8_t>> parts(nsuper);
    e.grp.assign(ngroups, 0);
    parallel_for(nsuper, n > (1u << 22) ? threads : 1, [&](std::uint64_t s, int) {
        const std::uint64_t b = s * super_pos, en = std::min(n, b + super_pos);
        std::uint16_t* go = e.grp.data() + (b >> GROUP_SHIFT);
        for (std::uint64_t gs = b; gs < en; gs += GROUP) {
            if (parts[s].size() > 0xFFFF) throw std::runtime_error("egdb: superblock overflow");
            *go++ = std::uint16_t(parts[s].size());
            fn(gs, std::min<std::uint64_t>(gs + GROUP, en), parts[s]);
        }
    });
    e.sup.resize(nsuper);
    std::uint64_t total = 0;
    for (std::uint64_t s = 0; s < nsuper; ++s) {
        e.sup[s] = std::uint32_t(total);
        total += parts[s].size();
        if (total > 0xFFFFFFFFull) throw std::runtime_error("egdb: compressed subslice exceeds 4 GiB");
    }
    e.data.reserve(total + 8);
    for (auto& p : parts) e.data.insert(e.data.end(), p.begin(), p.end());
    return e;
}

}  // namespace

Encoded encode(const std::uint8_t* vals, std::uint64_t n, int threads) {
    Encoded e;
    e.size = n;
    // Constant (all known values equal) subslices need no data at all.
    unsigned seen = 0;
    for (std::uint64_t i = 0; i < n && std::popcount(seen) < 2; ++i)
        if (vals[i] != V_DC) seen |= 1u << vals[i];
    if (std::popcount(seen) < 2) {
        e.kind = Encoded::Constant;
        e.value = seen ? std::uint8_t(std::countr_zero(seen)) : V_DRAW;
        return e;
    }
    return encode_groups(n, threads, [&](std::uint64_t gs, std::uint64_t ge, std::vector<std::uint8_t>& out) {
        encode_group(vals, gs, ge, out);
    });
}

namespace mtc {

Encoded encode(const std::uint16_t* v, std::uint64_t n, int threads) {
    bool any = false;
    for (std::uint64_t i = 0; i < n && !any; ++i) any = v[i] != MTC_DC;
    if (!any) {
        Encoded e;
        e.size = n;
        e.kind = Encoded::Constant;
        e.value = 0;
        return e;
    }
    return encode_groups(n, threads, [&](std::uint64_t gs, std::uint64_t ge, std::vector<std::uint8_t>& out) {
        int last = 0;
        for (std::uint64_t p = gs; p < ge;) {
            std::uint64_t q = p;
            while (q < ge && (v[q] == MTC_DC || v[q] == last)) ++q;
            if (q > p) {
                std::uint64_t run = q - p;
                int k = 30;
                while (MRUN[k] > run) --k;
                out.push_back(std::uint8_t(224 + k));
                p += MRUN[k];
                continue;
            }
            const int val = v[p] > 255 ? 255 : v[p];
            if (val >= 224) out.push_back(255);
            out.push_back(std::uint8_t(val));
            last = val;
            ++p;
        }
    });
}

}  // namespace mtc

}  // namespace regicide::egdb::detail::codec
