// Compares every position with 2..4 pieces (both colours to move) against the Chinook database
// through Cake's db.c lookup code. Built and run by xcheck.sh.
//
// Chinook's values are only defined when neither side has a capture (see the comment at the top
// of db.c); such positions are skipped. Our database is probed through the public API, i.e. with
// colour normalisation exactly as the engine will use it.
#include <cstdio>
#include <cstdlib>

#include "egdb/egdb.hpp"
#include "egdb/build.hpp"
#include "egdb/verify.hpp"
#include "egdb/index.hpp"

extern "C" {
struct pos {
    unsigned int bm, bk, wm, wk;
};
int DBInit(void);
int DBLookup(struct pos p, int turn);
int logging = 0;
}

using namespace regicide;
using namespace regicide::egdb::detail;

namespace {
constexpr int CH_TIE = 0, CH_WIN = 1, CH_LOSS = 2;
constexpr int CH_WHITE = 0, CH_BLACK = 1;

// Cake bit i is PDN square 4*(i/4) + 4 - i%4.
unsigned to_cake(Bitboard b) {
    unsigned r = 0;
    for (int i = 0; i < 32; ++i)
        if (b & bit(from_pdn(4 * (i / 4) + 4 - i % 4))) r |= 1u << i;
    return r;
}

const char* name(egdb::Wld w) {
    switch (w) {
        case egdb::Wld::Win: return "win";
        case egdb::Wld::Draw: return "draw";
        case egdb::Wld::Loss: return "loss";
        default: return "unknown";
    }
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2 || !egdb::init(argv[1], 64)) {
        std::fprintf(stderr, "cannot load regicide egdb from %s\n", argc > 1 ? argv[1] : "?");
        return 2;
    }
    if (egdb::max_pieces() < 4) {
        std::fprintf(stderr, "regicide egdb has only %d pieces\n", egdb::max_pieces());
        return 2;
    }
    const int chinook_pieces = DBInit();
    std::printf("Chinook db loaded (%d pieces), regicide max_pieces %d\n", chinook_pieces, egdb::max_pieces());
    unsigned long long compared = 0, skipped = 0, bad = 0, counts[3] = {};
    for (int n = 2; n <= 4; ++n) {
        unsigned long long cmp_n = 0;
        for (const Material& m : materials_with(n))
            for (int x = 0; x < 7; ++x)
                for (int y = 0; y < 7; ++y) {
                    const SubDims d = make_dims(m, x, y);
                    for (std::uint64_t i = 0; i < d.size; ++i) {
                        Pos p;
                        if (!unindex(d, i, p)) continue;
                        // Canonical frame == real board with Black to move. Also test the rotated
                        // board with White to move, which must have the same value.
                        for (int side = 0; side < 2; ++side) {
                            const Pos r = side == 0 ? p : Pos{rev(p.om), rev(p.ok), rev(p.sm), rev(p.sk)};
                            const Color stm = side == 0 ? BLACK : WHITE;
                            // r: sm/sk = black pieces, om/ok = white pieces (real colours).
                            const Bitboard black = r.sm | r.sk, white = r.om | r.ok, kings = r.sk | r.ok;
                            if (has_capture(p) || opp_has_capture(p)) {
                                ++skipped;
                                continue;
                            }
                            struct pos cp{to_cake(r.sm), to_cake(r.sk), to_cake(r.om), to_cake(r.ok)};
                            const int ch = DBLookup(cp, stm == BLACK ? CH_BLACK : CH_WHITE);
                            const egdb::Wld want = ch == CH_WIN    ? egdb::Wld::Win
                                                   : ch == CH_LOSS ? egdb::Wld::Loss
                                                   : ch == CH_TIE  ? egdb::Wld::Draw
                                                                   : egdb::Wld::Unknown;
                            const egdb::Wld got = egdb::probe(black, white, kings, stm);
                            ++compared;
                            ++cmp_n;
                            if (got != egdb::Wld::Unknown) ++counts[int(got) - 1];
                            if (got != want && bad++ < 20)
                                std::printf("MISMATCH %s %s to move: chinook %s (%d), regicide %s\n",
                                            describe(p).c_str(), stm == BLACK ? "black" : "white", name(want), ch,
                                            name(got));
                        }
                    }
                }
        std::printf("%d pieces: %llu positions compared\n", n, cmp_n);
    }
    std::printf("compared %llu positions (W %llu D %llu L %llu), skipped %llu with a capture for either side, "
                "mismatches %llu\n",
                compared, counts[0], counts[1], counts[2], skipped, bad);
    return bad ? 1 : 0;
}
