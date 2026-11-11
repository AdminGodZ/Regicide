#pragma once
// Database relabelling of training games (truth relabel; E2 data). Shared by tools/truth.cpp and tests/relabel_test.cpp.
#include <cstdint>
#include <vector>

#include "datafile.hpp"
#include "egdb/egdb.hpp"

namespace regicide::truth {

// Database value from black's point of view: 1 / 0 / -1, or 2 when unknown.
inline int db_black(const DataRecord& r) {
    const egdb::Wld w = egdb::probe(r.black, r.white, r.kings, r.color());
    if (w == egdb::Wld::Unknown) return 2;
    const int s = w == egdb::Wld::Win ? 1 : w == egdb::Wld::Loss ? -1 : 0;
    return r.color() == BLACK ? s : -s;
}

struct RelabelCount {
    long changed = 0;        // records whose result changed
    long own = 0;            // records <= db pieces labelled with their own database value
    long carried = 0;        // records after the first covered one labelled with the nearest earlier covered value
    bool covered = false;    // the game reaches a position the database knows
};

// Relabels one game in place (2026-11-11; before, every record took the value of the game's first covered record, so a
// game that entered the database as a win and then gave it away kept "win" on its later, drawn, positions):
// - records before the game's first covered record (<= db pieces, database value known) take that record's value as
//   the game result, as before;
// - every covered record takes its own database value;
// - any other record after the first covered one takes the value of the nearest earlier covered record. Within a game
//   the piece count never rises, so these are only <= db-piece records the database does not know (if the database
//   has such gaps) and, in a legacy file split by heuristic, a record of the next game wrongly kept in this one.
// A game without a covered record is unchanged.
inline RelabelCount relabel_game(std::vector<DataRecord>& g, int db) {
    RelabelCount c;
    std::size_t first = g.size();
    int cur = 2;
    for (std::size_t i = 0; i < g.size(); ++i)
        if (g[i].pieces() <= db && (cur = db_black(g[i])) != 2) {
            first = i;
            break;
        }
    if (first == g.size()) return c;
    c.covered = true;
    for (std::size_t i = 0; i < g.size(); ++i) {
        int v = cur;
        if (i >= first) {
            const int own = g[i].pieces() <= db ? db_black(g[i]) : 2;
            if (own != 2) {
                v = cur = own;
                ++c.own;
            } else {
                ++c.carried;
            }
        }
        if (g[i].result != v) {
            g[i].result = std::int8_t(v);
            ++c.changed;
        }
    }
    return c;
}

}  // namespace regicide::truth
