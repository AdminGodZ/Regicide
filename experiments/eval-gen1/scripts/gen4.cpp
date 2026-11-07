// Prints one move sequence per distinct position after 4 plies from the start (first path found, DFS order).
#include <cstdio>
#include <set>
#include <string>
#include <tuple>
#include "checkers.hpp"
using namespace cbt;
std::set<std::string> seen;
void rec(const Board& b, int d, const std::string& seq) {
    if (d == 4) { if (seen.insert(b.fen()).second) std::printf("%s\n", seq.c_str()); return; }
    for (const Move& m : b.legal_moves()) rec(b.after(m), d + 1, seq.empty() ? move_to_pdn(m) : seq + " " + move_to_pdn(m));
}
int main() { rec(Board::start(), 0, ""); std::fprintf(stderr, "%zu positions\n", seen.size()); }
