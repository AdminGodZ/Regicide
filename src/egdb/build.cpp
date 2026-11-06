#include "egdb/build.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>

#include "egdb/codec.hpp"
#include "egdb/parallel.hpp"
#include "egdb/store.hpp"

namespace regicide::egdb::detail {

void log_line(const char* fmt, ...) {
    static std::mutex mu;
    std::lock_guard lock(mu);
    const std::time_t now = std::time(nullptr);
    char ts[32];
    std::strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    std::printf("[%s] ", ts);
    va_list ap;
    va_start(ap, fmt);
    std::vprintf(fmt, ap);
    va_end(ap);
    std::printf("\n");
    std::fflush(stdout);
}

std::vector<Material> materials_with(int n) {
    std::vector<Material> v;
    for (int a = 0; a <= n; ++a)
        for (int b = 0; a + b <= n; ++b)
            for (int c = 0; a + b + c <= n; ++c) {
                const Material m{a, b, c, n - a - b - c};
                if (m.a + m.b >= 1 && m.c + m.d >= 1 && m.a + m.b <= MAX_SIDE && m.c + m.d <= MAX_SIDE)
                    v.push_back(m);
            }
    return v;
}

namespace {

// ---- per-position state during generation (one byte) ------------------------------------------
//   0..63  undecided; value = number of successors not yet known to be wins for the opponent
//          (+1 if some converting move reaches a draw, so that such positions never become losses)
//   64..   decided states below; bit 7 marks capture positions (don't-cares in the final table)
// Frontier states carry the parity of the pass that will propagate them.
enum : std::uint8_t {
    S_INV = 64,
    S_DRAW = 65,
    S_WIN = 66,
    S_LOSS = 67,
    S_WF = 68,  // + 2 * parity: win, frontier
    S_LF = 69,  // + 2 * parity: loss, frontier
    S_CAP = 0x80,
};
constexpr int CHUNK_SHIFT = 12;
constexpr std::uint64_t CHUNK = 1ull << CHUNK_SHIFT;

using Clock = std::chrono::steady_clock;
double secs(Clock::time_point t0) { return std::chrono::duration<double>(Clock::now() - t0).count(); }

std::string pos_str(const Pos& p) {
    char buf[96];
    std::snprintf(buf, sizeof buf, "sm=%08x sk=%08x om=%08x ok=%08x", p.sm, p.sk, p.om, p.ok);
    return buf;
}

struct Failure {
    std::atomic<bool> flag{false};
    std::mutex mu;
    std::string msg;
    void set(const std::string& m) {
        std::lock_guard lock(mu);
        if (!flag.exchange(true)) msg = m;
    }
};

struct Work {
    SubDims dims;
    std::unique_ptr<std::uint8_t[]> st;
    std::unique_ptr<std::uint16_t[]> depth;  // MTC, optional
    std::unique_ptr<std::uint8_t[]> dirty[2];
    std::uint64_t nchunks = 0;

    Work(const SubDims& d, bool mtc) : dims(d) {
        st.reset(new std::uint8_t[d.size]);
        if (mtc) depth.reset(new std::uint16_t[d.size]);
        nchunks = (d.size + CHUNK - 1) >> CHUNK_SHIFT;
        for (auto& v : dirty) {
            v.reset(new std::uint8_t[nchunks]);
            std::fill_n(v.get(), nchunks, 0);
        }
    }
};

template <class T>
std::atomic_ref<T> aref(T& v) {
    return std::atomic_ref<T>(v);
}

// Resolve every position from information outside the subslice pair: captures, promotions and
// leading-rank-changing man moves are looked up in finished tables; moves inside the pair are counted.
void init_sub(Work& s, int threads, Failure& fail) {
    parallel_for(s.nchunks, threads, [&](std::uint64_t c, int) {
        if (fail.flag.load(std::memory_order_relaxed)) return;
        const std::uint64_t b = c << CHUNK_SHIFT, e = std::min(s.dims.size, b + CHUNK);
        bool frontier = false;
        Cursor cur(s.dims);
        cur.set(b);
        for (std::uint64_t i = b; i < e; ++i, cur.next()) {
            if (!cur.valid) {
                s.st[i] = S_INV;
                continue;
            }
            const Pos& p = cur.p;
            if (s.depth) s.depth[i] = 0;
            if (has_capture(p)) {
                const Wld v = probe_canon(p);
                if (v == Wld::Unknown) {
                    fail.set("missing capture successor for " + pos_str(p));
                    return;
                }
                s.st[i] = S_CAP | (v == Wld::Win ? S_WF : v == Wld::Loss ? S_LF : S_DRAW);
                frontier |= v != Wld::Draw;
                continue;
            }
            int inside = 0;
            bool draw = false, unknown = false;
            const bool win = for_each_quiet(p, [&](const Pos& q, Quiet k) {
                if (k == Quiet::King || (k == Quiet::Man && adv_up(q.om) == s.dims.x)) {
                    ++inside;
                    return false;
                }
                const Wld r = probe_canon(q);
                unknown |= r == Wld::Unknown;
                draw |= r == Wld::Draw;
                return r == Wld::Loss;
            });
            if (unknown && !win) {
                fail.set("missing successor for " + pos_str(p));
                return;
            }
            if (win) {
                s.st[i] = S_WF;
                frontier = true;
            } else if (inside == 0) {
                s.st[i] = draw ? S_DRAW : S_LF;
                frontier |= !draw;
            } else {
                s.st[i] = std::uint8_t(inside + draw);
            }
        }
        if (frontier) s.dirty[0][c] = 1;
    });
}

// Propagate the frontier of `s` (parity `par`) to predecessors in `partner`. Returns whether any
// partner position was decided.
bool propagate(Work& s, Work& partner, int par, std::uint16_t depth, int threads) {
    std::atomic<bool> found{false};
    const std::uint8_t wf = S_WF + 2 * par, lf = S_LF + 2 * par;
    const std::uint8_t nwf = S_WF + 2 * (par ^ 1), nlf = S_LF + 2 * (par ^ 1);
    parallel_for(s.nchunks, threads, [&](std::uint64_t c, int) {
        if (!aref(s.dirty[par][c]).load(std::memory_order_relaxed)) return;
        aref(s.dirty[par][c]).store(0, std::memory_order_relaxed);
        const std::uint64_t b = c << CHUNK_SHIFT, e = std::min(s.dims.size, b + CHUNK);
        bool any = false;
        for (std::uint64_t i = b; i < e; ++i) {
            const std::uint8_t v = aref(s.st[i]).load(std::memory_order_relaxed);
            const std::uint8_t base = v & 0x7F;
            if (base != wf && base != lf) continue;
            const bool win = base == wf;
            aref(s.st[i]).store(std::uint8_t((v & S_CAP) | (win ? S_WIN : S_LOSS)), std::memory_order_relaxed);
            if (s.depth) s.depth[i] = depth;
            Pos p;
            unindex(s.dims, i, p);
            for_each_unmove(p, [&](const Pos& q, bool man) {
                if (man && adv_down(q.sm) != partner.dims.x) return;
                const std::uint64_t j = index_of(partner.dims, q);
                auto r = aref(partner.st[j]);
                std::uint8_t cur = r.load(std::memory_order_relaxed);
                while (cur < S_INV) {
                    const std::uint8_t nv = !win ? nwf : cur == 1 ? nlf : std::uint8_t(cur - 1);
                    if (r.compare_exchange_weak(cur, nv, std::memory_order_relaxed)) {
                        if (nv >= S_INV) {
                            aref(partner.dirty[par ^ 1][j >> CHUNK_SHIFT]).store(1, std::memory_order_relaxed);
                            any = true;
                        }
                        break;
                    }
                }
            });
        }
        if (any) found.store(true, std::memory_order_relaxed);
    });
    return found.load();
}

struct PairResult {
    codec::Encoded wld[2], mtc[2];
    std::uint64_t counts[2][4] = {};  // win, draw, loss, capture-or-invalid
    int passes = 0;
    std::uint16_t max_depth = 0;
};

// Converts final states to codec values (in place) and encodes.
void finish(Work& w, PairResult& r, int k, bool mtc, int threads) {
    std::vector<std::uint16_t> mtcv;
    if (mtc) mtcv.resize(w.dims.size);
    std::uint64_t cnt[4] = {};
    std::uint16_t maxd = 0;
    std::mutex mu;
    parallel_for(w.nchunks, threads, [&](std::uint64_t c, int) {
        const std::uint64_t b = c << CHUNK_SHIFT, e = std::min(w.dims.size, b + CHUNK);
        std::uint64_t lc[4] = {};
        std::uint16_t md = 0;
        for (std::uint64_t i = b; i < e; ++i) {
            const std::uint8_t v = w.st[i];
            const std::uint8_t base = v & 0x7F;
            std::uint8_t out;
            if ((v & S_CAP) || base == S_INV)
                out = codec::V_DC;
            else if (base == S_WIN)
                out = codec::V_WIN;
            else if (base == S_LOSS)
                out = codec::V_LOSS;
            else
                out = codec::V_DRAW;  // S_DRAW or still undecided after the last pass
            ++lc[out];
            if (mtc) {
                const bool decided = out == codec::V_WIN || out == codec::V_LOSS;
                mtcv[i] = decided ? w.depth[i] : codec::mtc::MTC_DC;
                if (decided) md = std::max(md, w.depth[i]);
            }
            w.st[i] = out;
        }
        std::lock_guard lock(mu);
        for (int j = 0; j < 4; ++j) cnt[j] += lc[j];
        maxd = std::max(maxd, md);
    });
    for (int j = 0; j < 4; ++j) r.counts[k][j] = cnt[j];
    r.max_depth = std::max(r.max_depth, maxd);
    r.wld[k] = codec::encode(w.st.get(), w.dims.size, threads);
    if (mtc) r.mtc[k] = codec::mtc::encode(mtcv.data(), w.dims.size, threads);
}

PairResult solve_pair(const SubDims& da, const SubDims* db, bool mtc, int threads, Failure& fail) {
    PairResult r;
    Work wa(da, mtc);
    std::unique_ptr<Work> wbp = db ? std::make_unique<Work>(*db, mtc) : nullptr;
    Work& wb = db ? *wbp : wa;
    init_sub(wa, threads, fail);
    if (db && !fail.flag) init_sub(wb, threads, fail);
    if (fail.flag) return r;
    for (int par = 0, pass = 0;; par ^= 1, ++pass) {
        const auto d = std::uint16_t(std::min(pass, 0xFFFE));
        bool any = propagate(wa, wb, par, d, threads);
        if (db) any |= propagate(wb, wa, par, d, threads);
        if (!any) {
            r.passes = pass + 1;
            break;
        }
    }
    finish(wa, r, 0, mtc, threads);
    if (db) finish(wb, r, 1, mtc, threads);
    return r;
}

std::string part_path(const BuildOptions& o, TableKind k, Material m, int x, int y) {
    std::string n = slice_name(k, m);
    n.insert(n.size() - 5, "_" + std::to_string(x) + std::to_string(y));
    return o.dir + "/parts/" + n;
}

// Keeps the encoded subslices of slices under construction alive while they are registered.
struct Pending {
    std::vector<std::unique_ptr<codec::Encoded>> enc;
    const codec::Encoded* keep(codec::Encoded&& e) {
        enc.push_back(std::make_unique<codec::Encoded>(std::move(e)));
        return enc.back().get();
    }
};

}  // namespace

bool build_pair(const BuildOptions& o, Material ma) {
    const Material mb = ma.partner();
    const bool single = ma == mb;
    const bool mtc = ma.total() <= o.mtc_pieces;
    const auto t0 = Clock::now();
    std::error_code ec;
    std::filesystem::create_directories(o.dir + "/parts", ec);

    struct SubPair {
        int x, y;
    };
    std::vector<SubPair> todo;
    for (int x = 0; x < (ma.a ? 7 : 1); ++x)
        for (int y = 0; y < (ma.c ? 7 : 1); ++y) {
            if (make_dims(ma, x, y).size == 0 || (single && x > y)) continue;
            todo.push_back({x, y});
        }
    // A non-capturing man move only ever raises its side's leading rank: solve high ranks first.
    std::stable_sort(todo.begin(), todo.end(), [](SubPair l, SubPair r) { return l.x + l.y > r.x + r.y; });

    clear_slice(TableKind::Wld, ma);
    clear_slice(TableKind::Wld, mb);
    clear_slice(TableKind::Mtc, ma);
    clear_slice(TableKind::Mtc, mb);

    Pending pending;
    std::vector<std::string> parts;
    std::uint64_t total[4] = {}, positions = 0;
    int max_passes = 0;
    std::uint16_t max_depth = 0;
    const TableKind kinds[2] = {TableKind::Wld, TableKind::Mtc};
    for (const SubPair& sp : todo) {
        const SubDims da = make_dims(ma, sp.x, sp.y), db = make_dims(mb, sp.y, sp.x);
        const bool one = single && sp.x == sp.y;
        // Resume: reuse finished parts.
        std::vector<std::string> paths;
        for (int k = 0; k < (mtc ? 2 : 1); ++k) {
            paths.push_back(part_path(o, kinds[k], ma, sp.x, sp.y));
            if (!one) paths.push_back(part_path(o, kinds[k], mb, sp.y, sp.x));
        }
        bool have = true;
        for (const auto& p : paths) have &= std::filesystem::exists(p, ec);
        if (have) {
            bool ok = true;
            for (int k = 0, i = 0; k < (mtc ? 2 : 1); ++k) {
                ok &= load_file(paths[i++], kinds[k], ma, true);
                if (!one) ok &= load_file(paths[i++], kinds[k], mb, true);
            }
            if (ok) {
                parts.insert(parts.end(), paths.begin(), paths.end());
                continue;
            }
            log_line("warning: could not load parts for %s x%d y%d, rebuilding", slice_name(TableKind::Wld, ma).c_str(),
                     sp.x, sp.y);
        }
        const auto ts = Clock::now();
        Failure fail;
        PairResult r = solve_pair(da, one ? nullptr : &db, mtc, o.threads, fail);
        if (fail.flag) {
            log_line("error: %s", fail.msg.c_str());
            return false;
        }
        for (int s = 0; s < (one ? 1 : 2); ++s) {
            const Material m = s == 0 ? ma : mb;
            const int x = s == 0 ? sp.x : sp.y, y = s == 0 ? sp.y : sp.x;
            for (int k = 0; k < (mtc ? 2 : 1); ++k) {
                codec::Encoded& e = k == 0 ? r.wld[s] : r.mtc[s];
                const codec::Encoded* kept = pending.keep(std::move(e));
                register_sub(kinds[k], m, x, y, *kept);
                Sub sub = slice_tab(kinds[k], m)->sub[x][y];
                const std::string path = part_path(o, kinds[k], m, x, y);
                if (!write_slice_file(path, kinds[k], m, {{x, y, sub}})) {
                    log_line("error: cannot write %s", path.c_str());
                    return false;
                }
                parts.push_back(path);
            }
            for (int j = 0; j < 4; ++j) total[j] += r.counts[s][j];
        }
        positions += da.size + (one ? 0 : db.size);
        max_passes = std::max(max_passes, r.passes);
        max_depth = std::max(max_depth, r.max_depth);
        if (o.verbose && secs(ts) > 2.0)
            log_line("  %s x%d y%d: %llu positions, %d passes, %.1fs", slice_name(TableKind::Wld, ma).c_str(), sp.x,
                     sp.y, (unsigned long long)(da.size + (one ? 0 : db.size)), r.passes, secs(ts));
    }

    // Assemble the slice files from the (registered) subslices, then switch to the mapped files.
    for (int s = 0; s < (single ? 1 : 2); ++s) {
        const Material m = s == 0 ? ma : mb;
        for (int k = 0; k < (mtc ? 2 : 1); ++k) {
            std::vector<SubEntry> subs;
            const SliceTab* t = slice_tab(kinds[k], m);
            for (int x = 0; x < 7; ++x)
                for (int y = 0; y < 7; ++y)
                    if (t && t->sub[x][y].kind) subs.push_back({x, y, t->sub[x][y]});
            const std::string path = o.dir + "/" + slice_name(kinds[k], m);
            if (!write_slice_file(path, kinds[k], m, subs) || !load_file(path, kinds[k], m)) {
                log_line("error: cannot write or reload %s", path.c_str());
                return false;
            }
        }
    }
    for (const auto& p : parts) std::filesystem::remove(p, ec);
    std::uint64_t bytes = 0;
    for (int s = 0; s < (single ? 1 : 2); ++s)
        bytes += std::filesystem::file_size(o.dir + "/" + slice_name(TableKind::Wld, s == 0 ? ma : mb), ec);
    log_line("%s%s%s: %llu positions (W %llu D %llu L %llu, cap/inv %llu), max passes %d%s, %llu bytes, %.1fs",
             slice_name(TableKind::Wld, ma).c_str() + 4, single ? "" : " + ",
             single ? "" : slice_name(TableKind::Wld, mb).c_str() + 4, (unsigned long long)positions,
             (unsigned long long)total[0], (unsigned long long)total[1], (unsigned long long)total[2],
             (unsigned long long)total[3], max_passes,
             mtc ? (", max mtc " + std::to_string(max_depth)).c_str() : "", (unsigned long long)bytes, secs(t0));
    return true;
}

bool build_range(const BuildOptions& o, int min_pieces, int max_pieces) {
    std::error_code ec;
    std::filesystem::create_directories(o.dir, ec);
    load_dir(o.dir, 0);
    for (int n = 2; n <= max_pieces; ++n) {
        std::vector<Material> pairs;
        for (const Material& m : materials_with(n)) {
            const Material p = m.partner();
            if (std::make_tuple(p.a, p.b, p.c, p.d) < std::make_tuple(m.a, m.b, m.c, m.d)) continue;
            pairs.push_back(m);
        }
        // Promotions turn a man into a king: slices with fewer men must be finished first.
        std::stable_sort(pairs.begin(), pairs.end(), [](Material l, Material r) { return l.a + l.c < r.a + r.c; });
        const auto t0 = Clock::now();
        int built = 0;
        for (const Material& m : pairs) {
            const bool mtc = n <= o.mtc_pieces;
            const bool done = slice_complete(TableKind::Wld, m) && slice_complete(TableKind::Wld, m.partner()) &&
                              (!mtc || (slice_complete(TableKind::Mtc, m) && slice_complete(TableKind::Mtc, m.partner())));
            if (done) continue;
            if (n < min_pieces) {
                log_line("error: %d-piece slice %s missing; build smaller slices first", n,
                         slice_name(TableKind::Wld, m).c_str());
                return false;
            }
            if (!build_pair(o, m)) return false;
            ++built;
        }
        if (built) log_line("=== %d pieces done: %d slice pairs built in %.1fs", n, built, secs(t0));
    }
    return true;
}

}  // namespace regicide::egdb::detail
