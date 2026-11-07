#include "egdb/verify.hpp"

#include <algorithm>
#include <mutex>
#include <vector>

#include "egdb/build.hpp"
#include "egdb/parallel.hpp"
#include "egdb/store.hpp"

namespace regicide::egdb::detail {

void VerifyStats::merge(const VerifyStats& o) {
    checked += o.checked;
    captures += o.captures;
    wld_bad += o.wld_bad;
    mtc_checked += o.mtc_checked;
    mtc_bad += o.mtc_bad;
    for (int i = 0; i < 3; ++i) counts[i] += o.counts[i];
    if (first_error.empty()) first_error = o.first_error;
}

std::string describe(const Pos& p) {
    std::string s = "B:W";
    auto list = [&](Bitboard men, Bitboard kings) {
        bool first = true;
        for (int n = 1; n <= 32; ++n) {
            const int sq = from_pdn(n);
            if (!((men | kings) & bit(sq))) continue;
            if (!first) s += ',';
            first = false;
            if (kings & bit(sq)) s += 'K';
            s += std::to_string(n);
        }
    };
    list(p.om, p.ok);
    s += ":B";
    list(p.sm, p.sk);
    return s;
}

namespace {

std::uint64_t splitmix(std::uint64_t& s) {
    std::uint64_t z = (s += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

void check_position(const Pos& p, bool check_mtc, VerifyStats& st) {
    if (has_capture(p)) {
        ++st.captures;
        return;
    }
    const Wld stored = lookup_wld(p);
    bool any_draw = false, any_loss = false, unknown = false;
    const int x = adv_x(p);
    // MTC recurrence: converting moves count 0, others 1 + MTC(successor).
    int best_win = 1 << 30, worst_loss = 0;
    for_each_quiet(p, [&](const Pos& q, Quiet k) {
        const Wld r = probe_canon(q);
        unknown |= r == Wld::Unknown;
        any_draw |= r == Wld::Draw;
        any_loss |= r == Wld::Loss;
        if (check_mtc && r != Wld::Draw && r != Wld::Unknown) {
            const bool converting = !(k == Quiet::King || (k == Quiet::Man && adv_up(q.om) == x));
            const int cand = converting ? 0 : 1 + probe_mtc_canon(q);
            if (r == Wld::Loss) best_win = std::min(best_win, cand);
            if (r == Wld::Win) worst_loss = std::max(worst_loss, cand);
        }
        return false;
    });
    const Wld expect = unknown ? Wld::Unknown : any_loss ? Wld::Win : any_draw ? Wld::Draw : Wld::Loss;
    ++st.checked;
    if (stored != Wld::Unknown) ++st.counts[int(stored) - 1];
    if (stored != expect || stored == Wld::Unknown) {
        if (!st.wld_bad++)
            st.first_error = describe(p) + ": stored " + std::to_string(int(stored)) + " expected " +
                             std::to_string(int(expect));
        return;
    }
    if (check_mtc && stored != Wld::Draw) {
        const int m = lookup_mtc(p);
        const int want = stored == Wld::Win ? best_win : worst_loss;
        ++st.mtc_checked;
        if (m != std::min(want, 255) && !(m == 255 && want >= 255)) {
            if (!st.mtc_bad++)
                st.first_error = describe(p) + ": stored mtc " + std::to_string(m) + " expected " +
                                 std::to_string(want);
        }
    }
}

}  // namespace

VerifyStats verify_slice(Material m, std::uint64_t samples, std::uint64_t seed, int threads, bool check_mtc) {
    std::vector<SubDims> subs;
    std::vector<std::uint64_t> cum;
    std::uint64_t total = 0;
    for (int x = 0; x < (m.a ? 7 : 1); ++x)
        for (int y = 0; y < (m.c ? 7 : 1); ++y) {
            const SubDims d = make_dims(m, x, y);
            if (!d.size) continue;
            subs.push_back(d);
            total += d.size;
            cum.push_back(total);
        }
    const std::uint64_t n = samples ? samples : total;
    const std::uint64_t chunks = (n + 4095) / 4096;
    VerifyStats all;
    std::mutex mu;
    parallel_for(chunks, threads, [&](std::uint64_t c, int) {
        VerifyStats st;
        std::uint64_t rng = seed ^ (c * 0x632be59bd9b4e019ull);
        const std::uint64_t b = c * 4096, e = std::min(n, b + 4096);
        for (std::uint64_t i = b; i < e; ++i) {
            const std::uint64_t g = samples ? splitmix(rng) % total : i;
            const std::size_t s = std::upper_bound(cum.begin(), cum.end(), g) - cum.begin();
            const std::uint64_t idx = g - (s ? cum[s - 1] : 0);
            Pos p;
            if (!unindex(subs[s], idx, p)) continue;
            check_position(p, check_mtc, st);
        }
        std::lock_guard lock(mu);
        all.merge(st);
    });
    return all;
}

namespace {
std::uint64_t men_pairs(int a, int c) {  // disjoint (side-to-move men, opponent men) placements
    std::uint64_t r = 0;
    for (int i = 0; i <= std::min(a, 24); ++i) r += binom(4, a - i) * binom(24, i) * binom(28 - i, c);
    return r;
}
}  // namespace

std::uint64_t count_positions_formula(int n) {
    std::uint64_t total = 0;
    for (int a = 0; a <= n; ++a)
        for (int b = 0; a + b <= n; ++b)
            for (int c = 0; a + b + c <= n; ++c) {
                const int d = n - a - b - c;
                total += men_pairs(a, c) * binom(32 - a - c, b) * binom(32 - a - c - b, d);
            }
    return total;
}

std::uint64_t count_positions_index(int n) {
    std::uint64_t total = 0;
    for (const Material& m : materials_with(n)) {
        for (int x = 0; x < (m.a ? 7 : 1); ++x)
            for (int y = 0; y < (m.c ? 7 : 1); ++y) {
                const SubDims d = make_dims(m, x, y);
                if (!d.size) continue;
                const std::uint32_t* us = unrank_table(m.a) + men_base(m.a, x);
                const std::uint32_t* uo = unrank_table(m.c) + men_base(m.c, y);
                std::uint64_t pairs = 0;
                for (std::uint64_t i = 0; i < d.ns; ++i) {
                    const Bitboard sm = rev(us[i]);
                    for (std::uint64_t j = 0; j < d.no; ++j) pairs += (sm & uo[j]) == 0;
                }
                total += pairs * d.nks * d.nko;
            }
    }
    // Placements in which one side has no pieces are not part of the database.
    for (int a = 0; a <= n; ++a) {
        total += men_pairs(a, 0) * binom(32 - a, n - a);  // only "side to move" pieces
        total += men_pairs(0, a) * binom(32 - a, n - a);  // only opponent pieces
    }
    return total;
}

}  // namespace regicide::egdb::detail
