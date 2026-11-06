// Endgame database generator and maintenance tool.
//
//   egdb_gen build  --max N [--min N] [--mtc N] [--threads T] [--dir D]
//   egdb_gen verify --pieces N [--samples K] [--mtc] [--threads T] [--dir D] [--slice abcd]
//   egdb_gen count  [--max N]
//   egdb_gen stats  [--dir D]
//   egdb_gen bench  [--pieces N] [--threads T] [--seconds S] [--dir D]
//
// The default directory is $HOME/regicide-data/egdb. `build` is resumable: finished slices are
// kept, and finished subslices of an interrupted slice are kept in <dir>/parts. Run long builds
// in the background, e.g.  nohup egdb_gen build --max 7 > gen7.log 2>&1 &

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "egdb/build.hpp"
#include "egdb/egdb.hpp"
#include "egdb/store.hpp"
#include "egdb/verify.hpp"

using namespace regicide;
using namespace regicide::egdb::detail;

namespace {

struct Args {
    std::string cmd;
    std::string dir;
    std::string slice;
    int max = 0, min = 2, mtc = 0, threads = 8, pieces = 0;
    std::uint64_t samples = 100000;
    double seconds = 3.0;
    bool check_mtc = false;
};

std::string default_dir() {
    const char* home = std::getenv("HOME");
    return std::string(home ? home : ".") + "/regicide-data/egdb";
}

bool parse(int argc, char** argv, Args& a) {
    if (argc < 2) return false;
    a.cmd = argv[1];
    a.dir = default_dir();
    for (int i = 2; i < argc; ++i) {
        const std::string k = argv[i];
        auto val = [&]() -> const char* { return i + 1 < argc ? argv[++i] : ""; };
        if (k == "--dir") a.dir = val();
        else if (k == "--max") a.max = std::atoi(val());
        else if (k == "--min") a.min = std::atoi(val());
        else if (k == "--mtc" && a.cmd == "build") a.mtc = std::atoi(val());
        else if (k == "--mtc") a.check_mtc = true;
        else if (k == "--threads") a.threads = std::atoi(val());
        else if (k == "--pieces") a.pieces = std::atoi(val());
        else if (k == "--samples") a.samples = std::strtoull(val(), nullptr, 10);
        else if (k == "--seconds") a.seconds = std::atof(val());
        else if (k == "--slice") a.slice = val();
        else return false;
    }
    return true;
}

int cmd_build(const Args& a) {
    BuildOptions o;
    o.dir = a.dir;
    o.threads = a.threads;
    o.mtc_pieces = a.mtc;
    log_line("building %d..%d pieces into %s with %d threads (mtc up to %d pieces)", a.min, a.max, a.dir.c_str(),
             a.threads, a.mtc);
    const auto t0 = std::chrono::steady_clock::now();
    const bool ok = build_range(o, a.min, a.max);
    log_line("%s after %.1fs", ok ? "finished" : "FAILED",
             std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
    return ok ? 0 : 1;
}

int cmd_verify(const Args& a) {
    load_dir(a.dir, 0);
    std::vector<Material> mats;
    if (!a.slice.empty() && a.slice.size() == 4)
        mats.push_back({a.slice[0] - '0', a.slice[1] - '0', a.slice[2] - '0', a.slice[3] - '0'});
    else
        mats = materials_with(a.pieces);
    VerifyStats total;
    for (const Material& m : mats) {
        const VerifyStats s = verify_slice(m, a.samples, 12345 + m.a * 1000 + m.b * 100 + m.c * 10 + m.d, a.threads,
                                           a.check_mtc);
        std::printf("%s: checked %llu (captures %llu) W %llu D %llu L %llu  wld-bad %llu  mtc %llu bad %llu %s\n",
                    slice_name(TableKind::Wld, m).c_str(), (unsigned long long)s.checked,
                    (unsigned long long)s.captures, (unsigned long long)s.counts[0], (unsigned long long)s.counts[1],
                    (unsigned long long)s.counts[2], (unsigned long long)s.wld_bad, (unsigned long long)s.mtc_checked,
                    (unsigned long long)s.mtc_bad, s.first_error.c_str());
        std::fflush(stdout);
        total.merge(s);
    }
    std::printf("TOTAL: checked %llu, wld mismatches %llu, mtc checked %llu, mtc mismatches %llu\n",
                (unsigned long long)total.checked, (unsigned long long)total.wld_bad,
                (unsigned long long)total.mtc_checked, (unsigned long long)total.mtc_bad);
    return total.wld_bad || total.mtc_bad ? 1 : 0;
}

int cmd_count(const Args& a) {
    // Schaeffer et al., "Solving Checkers" / Chinook: positions by number of pieces.
    static const std::uint64_t known[] = {0, 120, 6972, 261224, 7092774, 148688232, 2503611964ull,
                                          34779531480ull, 406309208481ull};
    int bad = 0;
    for (int n = 1; n <= (a.max ? a.max : 8); ++n) {
        const std::uint64_t f = count_positions_formula(n);
        const std::uint64_t ix = n >= 2 ? count_positions_index(n) : f;
        std::printf("%d pieces: formula %llu, index %llu, known %llu %s\n", n, (unsigned long long)f,
                    (unsigned long long)ix, (unsigned long long)known[n], f == known[n] && ix == f ? "ok" : "MISMATCH");
        bad += !(f == known[n] && ix == f);
    }
    return bad ? 1 : 0;
}

int cmd_stats(const Args& a) {
    namespace fs = std::filesystem;
    std::map<int, std::uint64_t> bytes[2], positions;
    for (const auto& e : fs::directory_iterator(a.dir)) {
        const std::string n = e.path().filename().string();
        int pa, pb, pc, pd;
        char kind[4] = {};
        if (std::sscanf(n.c_str(), "%3[a-z]_%1d%1d%1d%1d.rgdb", kind, &pa, &pb, &pc, &pd) != 5) continue;
        const int t = pa + pb + pc + pd;
        const bool mtc = std::strcmp(kind, "mtc") == 0;
        bytes[mtc][t] += e.file_size();
        if (!mtc)
            for (int x = 0; x < 7; ++x)
                for (int y = 0; y < 7; ++y) positions[t] += make_dims({pa, pb, pc, pd}, x, y).size;
    }
    for (auto [t, b] : bytes[0])
        std::printf("%d pieces: %llu index entries, WLD %.2f MiB (%.4f bits/entry), MTC %.2f MiB\n", t,
                    (unsigned long long)positions[t], b / 1048576.0, 8.0 * b / positions[t], bytes[1][t] / 1048576.0);
    return 0;
}

// Probe throughput on random positions with `pieces` pieces (side to move random).
int cmd_bench(const Args& a) {
    egdb::init(a.dir, 1 << 20);
    const int maxp = egdb::max_pieces();
    const int n = a.pieces ? a.pieces : maxp;
    std::printf("max_pieces %d, benchmarking %d-piece positions\n", maxp, n);
    struct Q {
        Bitboard b, w, k;
        Color stm;
    };
    std::vector<Q> qs;
    std::uint64_t rng = 42;
    auto rnd = [&] {
        rng ^= rng << 13, rng ^= rng >> 7, rng ^= rng << 17;
        return rng;
    };
    while (qs.size() < (1u << 20)) {
        Bitboard occ = 0, b = 0, w = 0, k = 0;
        for (int i = 0; i < n; ++i) {
            int sq;
            do sq = int(rnd() % 32);
            while (occ & bit(sq));
            occ |= bit(sq);
            const bool black = i == 0 ? true : i == 1 ? false : rnd() & 1;
            const bool king = rnd() % 3 == 0 || (black && sq < 4) || (!black && sq >= 28);
            (black ? b : w) |= bit(sq);
            if (king) k |= bit(sq);
        }
        qs.push_back({b, w, k, Color(rnd() & 1)});
    }
    for (int t : {1, a.threads}) {
        std::atomic<std::uint64_t> done{0}, unknown{0};
        std::atomic<bool> stop{false};
        std::vector<std::jthread> pool;
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < t; ++i)
            pool.emplace_back([&, i] {
                std::uint64_t cnt = 0, unk = 0;
                std::size_t j = std::size_t(i) * 7919;
                while (!stop.load(std::memory_order_relaxed)) {
                    for (int r = 0; r < 4096; ++r, ++j) {
                        const Q& q = qs[j & (qs.size() - 1)];
                        unk += egdb::probe(q.b, q.w, q.k, q.stm) == egdb::Wld::Unknown;
                    }
                    cnt += 4096;
                }
                done += cnt;
                unknown += unk;
            });
        std::this_thread::sleep_for(std::chrono::duration<double>(a.seconds));
        stop = true;
        pool.clear();
        const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::printf("%2d thread(s): %.2f M probes/s (%llu unknown)\n", t, done / s / 1e6,
                    (unsigned long long)unknown.load());
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    Args a;
    if (!parse(argc, argv, a)) {
        std::fprintf(stderr,
                     "usage: egdb_gen build --max N [--min N] [--mtc N] [--threads T] [--dir D]\n"
                     "       egdb_gen verify --pieces N [--samples K (0 = all)] [--mtc] [--slice abcd]\n"
                     "       egdb_gen count [--max N] | stats | bench [--pieces N] [--threads T]\n");
        return 2;
    }
    if (a.cmd == "build") return cmd_build(a);
    if (a.cmd == "verify") return cmd_verify(a);
    if (a.cmd == "count") return cmd_count(a);
    if (a.cmd == "stats") return cmd_stats(a);
    if (a.cmd == "bench") return cmd_bench(a);
    std::fprintf(stderr, "unknown command %s\n", a.cmd.c_str());
    return 2;
}
