// Endgame database tests.
//
//   egdb_test          self-contained: movegen perft, index bijection and position counts, a fresh
//                      2..4-piece build (with MTC) verified exhaustively, known results.
//   egdb_test --data   checks the generated database in $REGICIDE_EGDB or $HOME/regicide-data/egdb
//                      (sampled one-ply consistency for every slice, known results). Skips if absent.
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "egdb/board.hpp"
#include "egdb/build.hpp"
#include "egdb/egdb.hpp"
#include "egdb/index.hpp"
#include "egdb/store.hpp"
#include "egdb/verify.hpp"

using namespace regicide;
using namespace regicide::egdb::detail;
using egdb::Wld;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
    std::fflush(stdout);
    failures += !ok;
}

// Perft on canonical positions; identical successor positions are counted once (equivalent to the
// engine's (from, to, captures) deduplication).
std::uint64_t perft(const Pos& p, int depth) {
    if (depth == 0) return 1;
    std::vector<Pos> succ;
    if (has_capture(p)) {
        for_each_capture(p, [&](const Pos& q) {
            succ.push_back(q);
            return false;
        });
        std::set<std::tuple<Bitboard, Bitboard, Bitboard, Bitboard>> seen;
        std::vector<Pos> uniq;
        for (const Pos& q : succ)
            if (seen.insert({q.sm, q.sk, q.om, q.ok}).second) uniq.push_back(q);
        succ.swap(uniq);
    } else {
        for_each_quiet(p, [&](const Pos& q, Quiet) {
            succ.push_back(q);
            return false;
        });
    }
    if (depth == 1) return succ.size();
    std::uint64_t n = 0;
    for (const Pos& q : succ) n += perft(q, depth - 1);
    return n;
}

void test_perft() {
    constexpr std::uint64_t expected[] = {1, 7, 49, 302, 1469, 7361, 36768, 179740, 845931, 3963680, 18391564};
    const Pos start = from_colors(0x00000FFFu << 20, 0x00000FFFu, 0, BLACK);  // black on rows 5..7
    for (int d = 0; d <= 10; ++d) {
        const std::uint64_t n = perft(start, d);
        check(n == expected[d], "perft(" + std::to_string(d) + ") = " + std::to_string(n));
    }
}

void test_index() {
    // Bijection: every valid index decodes to a position that encodes back to the same index.
    std::uint64_t bad = 0, valid = 0;
    for (int n = 2; n <= 4; ++n)
        for (const Material& m : materials_with(n))
            for (int x = 0; x < 7; ++x)
                for (int y = 0; y < 7; ++y) {
                    const SubDims d = make_dims(m, x, y);
                    for (std::uint64_t i = 0; i < d.size; ++i) {
                        Pos p;
                        if (!unindex(d, i, p)) continue;
                        ++valid;
                        const bool ok = material_of(p) == m && adv_x(p) == x && adv_y(p) == y &&
                                        index_of(d, p) == i && !(p.sm & 0xFu) && !(p.om & 0xF0000000u) &&
                                        std::popcount(p.occ()) == n;
                        bad += !ok;
                    }
                }
    check(bad == 0, "index round trip for 2..4 pieces (" + std::to_string(valid) + " positions)");
    static const std::uint64_t known[] = {0, 120, 6972, 261224, 7092774, 148688232, 2503611964ull};
    for (int n = 2; n <= 6; ++n) {
        const std::uint64_t f = count_positions_formula(n), ix = count_positions_index(n);
        check(f == known[n] && ix == known[n], std::to_string(n) + "-piece position count " + std::to_string(ix));
    }
}

Wld probe_fen(const char* fen) {
    // Minimal PDN FEN parser: "B:W18,K23:BK5" (side to move first).
    Bitboard b = 0, w = 0, k = 0;
    const Color stm = fen[0] == 'W' ? WHITE : BLACK;
    Bitboard* side = nullptr;
    for (const char* s = fen + 1; *s;) {
        if (*s == 'W' || *s == 'B') {
            side = *s == 'W' ? &w : &b;
            ++s;
        } else if (*s == 'K' || (*s >= '0' && *s <= '9')) {
            const bool king = *s == 'K';
            if (king) ++s;
            const int n = std::atoi(s);
            while (*s >= '0' && *s <= '9') ++s;
            *side |= bit(from_pdn(n));
            if (king) k |= bit(from_pdn(n));
        } else {
            ++s;
        }
    }
    return egdb::probe(b, w, k, stm);
}

void test_known_results(int max_pieces) {
    // 2 kings vs 1 king: the two kings win, apart from a handful of positions where the lone king
    // forks or traps a king. The lone king never wins.
    {
        std::uint64_t n = 0, strong_wins = 0, weak_wins = 0;
        for (int strong_moves = 0; strong_moves < 2; ++strong_moves) {
            const SubDims d = make_dims(strong_moves ? Material{0, 2, 0, 1} : Material{0, 1, 0, 2}, 0, 0);
            for (std::uint64_t i = 0; i < d.size; ++i) {
                Pos p;
                unindex(d, i, p);
                if (has_capture(p) || opp_has_capture(p)) continue;
                const Wld v = probe_canon(p);
                ++n;
                strong_wins += v == (strong_moves ? Wld::Win : Wld::Loss);
                weak_wins += v == (strong_moves ? Wld::Loss : Wld::Win);
            }
        }
        check(weak_wins == 0 && strong_wins * 1000 > n * 995,
              "2 kings beat 1 king: " + std::to_string(strong_wins) + " of " + std::to_string(n) +
                  " quiet positions won, lone king never wins");
    }
    // 1 king vs 1 king: a king in a double corner (PDN 1, 5, 28, 32) can never be beaten; most other
    // positions are drawn too, but a king caught away from the double corners can be trapped.
    {
        const Bitboard dc = bit(from_pdn(1)) | bit(from_pdn(5)) | bit(from_pdn(28)) | bit(from_pdn(32));
        std::uint64_t n = 0, bad = 0, draws = 0, quiet = 0;
        const SubDims d = make_dims({0, 1, 0, 1}, 0, 0);
        for (std::uint64_t i = 0; i < d.size; ++i) {
            Pos p;
            unindex(d, i, p);
            if (has_capture(p) || opp_has_capture(p)) continue;
            const Wld v = probe_canon(p);
            ++quiet;
            draws += v == Wld::Draw;
            if (p.sk & dc) ++n, bad += v == Wld::Loss;
            if (p.ok & dc) ++n, bad += v == Wld::Win;
        }
        check(n == 240 && bad == 0 && draws * 2 > quiet,
              "1 king vs 1 king: double corner always holds, " + std::to_string(draws) + " of " +
                  std::to_string(quiet) + " quiet positions drawn");
    }
    check(probe_fen("B:WK27:BK6") == Wld::Draw, "K vs K far apart is a draw");
    // Black king on a1 (PDN 29) must go to b2 (25), where the white king on c3 (22) captures it.
    check(probe_fen("B:WK22:BK29") == Wld::Loss, "K vs K: trapped single-corner king loses");
    check(probe_fen("W:WK4:BK11") == Wld::Loss, "K vs K: same trap rotated, white to move loses");
    // Pending captures are resolved inside probe().
    check(probe_fen("B:W18:B14") == Wld::Win, "black man captures the last white man (14x23)");
    check(probe_fen("W:W18:B14") == Wld::Win, "white man captures the last black man (18x9)");
    check(probe_fen("B:WK18:BK14") == Wld::Win, "adjacent kings: side to move captures");
    if (max_pieces >= 3) {
        // The basic 2-vs-1 kings win, white to move, with white having the two kings.
        check(probe_fen("W:WK1,K2:BK32") == Wld::Win, "two kings vs one, strong side to move");
        check(probe_fen("B:WK1,K2:BK32") == Wld::Loss, "two kings vs one, weak side to move");
    }
}

std::string temp_dir() {
    std::string base = (std::filesystem::temp_directory_path() /
                        ("regicide-egdb-test-" + std::to_string(std::random_device{}()))).string();
    std::filesystem::remove_all(base);
    return base;
}

int self_test() {
    test_perft();
    test_index();

    const std::string dir = temp_dir();
    BuildOptions o;
    o.dir = dir;
    o.threads = 4;
    o.mtc_pieces = 4;
    o.verbose = false;
    check(build_range(o, 2, 4), "build 2..4 pieces into " + dir);
    check(egdb::max_pieces() == 4 && egdb::max_mtc_pieces() == 4, "max_pieces() == 4");

    // Exhaustive consistency: every stored value equals the minimax over successor values, and MTC
    // satisfies its recurrence.
    VerifyStats all;
    for (int n = 2; n <= 4; ++n)
        for (const Material& m : materials_with(n)) all.merge(verify_slice(m, 0, 1, 4, true));
    check(all.wld_bad == 0 && all.checked > 5000000,
          "exhaustive minimax consistency: " + std::to_string(all.checked) + " positions " + all.first_error);
    check(all.mtc_bad == 0 && all.mtc_checked > 3000000,
          "exhaustive MTC consistency: " + std::to_string(all.mtc_checked) + " positions");
    test_known_results(4);

    // Resumability: deleting one slice file rebuilds exactly that slice pair.
    const std::string victim = dir + "/" + slice_name(TableKind::Wld, {1, 1, 1, 1});
    std::filesystem::remove(victim);
    clear_slice(TableKind::Wld, {1, 1, 1, 1});
    check(build_range(o, 2, 4) && std::filesystem::exists(victim), "resume rebuilds a missing slice");
    VerifyStats again = verify_slice({1, 1, 1, 1}, 0, 2, 4, true);
    check(again.wld_bad == 0 && again.mtc_bad == 0, "rebuilt slice verifies");
    std::filesystem::remove_all(dir);
    return failures;
}

int data_test() {
    const char* env = std::getenv("REGICIDE_EGDB");
    const char* home = std::getenv("HOME");
    const std::string dir = env ? env : std::string(home ? home : ".") + "/regicide-data/egdb";
    if (!egdb::init(dir, 4096) || egdb::max_pieces() < 2) {
        std::printf("skip: no database in %s\n", dir.c_str());
        return 0;
    }
    const int maxp = egdb::max_pieces(), maxm = egdb::max_mtc_pieces();
    std::printf("database %s: max_pieces %d, max_mtc_pieces %d\n", dir.c_str(), maxp, maxm);
    for (int n = 2; n <= maxp; ++n) {
        VerifyStats s;
        for (const Material& m : materials_with(n)) s.merge(verify_slice(m, 4000, 7 + n, 8, n <= maxm));
        check(s.wld_bad == 0 && s.mtc_bad == 0,
              std::to_string(n) + " pieces: " + std::to_string(s.checked) + " sampled positions consistent" +
                  (n <= maxm ? " (incl. MTC)" : "") + " " + s.first_error);
    }
    test_known_results(maxp);
    return failures;
}

}  // namespace

int main(int argc, char** argv) {
    const bool data = argc > 1 && std::string(argv[1]) == "--data";
    const int f = data ? data_test() : self_test();
    std::printf("%s\n", f ? "FAILED" : "all egdb tests passed");
    return f ? 1 : 0;
}
