// truth relabel (tools/relabel.hpp, 2026-11-11): every record with <= db pieces gets its own database value. Before,
// the whole game took the value of its first covered record, so B:WK22:BK14 (a draw, reached after the win
// B:WK13:BK14,K22 was given away by 22-17 13x22) was written as a Black win.
#include <cstdio>
#include <string>
#include <vector>

#include "datafile.hpp"
#include "position.hpp"
#include "relabel.hpp"
#include "test_egdb.hpp"

using namespace regicide;

namespace {

DataRecord rec(const char* fen, int result) {
    Position p;
    if (!p.set_fen(fen)) {
        std::printf("bad FEN %s\n", fen);
        std::exit(1);
    }
    DataRecord r = DataRecord::make(p, 0);
    r.result = std::int8_t(result);
    return r;
}

int fails = 0;
void check(bool ok, const char* what) {
    std::printf("%s: %s\n", ok ? "ok" : "FAIL", what);
    fails += !ok;
}

}  // namespace

int main() {
    const std::string dir = test_egdb::build("relabel", 3);
    if (dir.empty()) return std::printf("cannot build the test database\n"), 1;
    check(truth::db_black(rec("B:WK13:BK14,K22", 0)) == 1, "B:WK13:BK14,K22 is a Black win");
    check(truth::db_black(rec("B:WK22:BK14", 0)) == 0, "B:WK22:BK14 is a draw");
    // A game: one record above the database (stored as a White win), the won 3-piece position, the drawn 2-piece one.
    std::vector<DataRecord> g{rec("W:WK13,K30:BK14,K22", -1), rec("B:WK13:BK14,K22", -1), rec("B:WK22:BK14", -1)};
    const truth::RelabelCount c = truth::relabel_game(g, 3);
    check(g[0].result == 1, "the record before the database takes the first covered value (Black win)");
    check(g[1].result == 1, "the covered win keeps its own value");
    check(g[2].result == 0, "the later drawn position gets its own value, a draw (was: Black win)");
    check(c.changed == 3 && c.own == 2 && c.carried == 0, "counts: 3 changed, 2 own values, 0 carried");
    // A game that never reaches the database is unchanged.
    std::vector<DataRecord> h{rec("W:WK13,K30:BK14,K22", -1)};
    check(truth::relabel_game(h, 3).changed == 0 && h[0].result == -1, "a game without a covered record is unchanged");
    test_egdb::remove(dir);
    std::printf("%s\n", fails ? "relabel_test FAILED" : "relabel_test passed");
    return fails ? 1 : 0;
}
