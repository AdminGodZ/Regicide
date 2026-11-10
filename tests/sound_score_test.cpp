// SearchResult::sound_score: a losing score from a cut-short iteration only says that the moves searched so far
// lose, so it falls back to the last completed score, or to a draw if no iteration was completed. Wins, and every
// completed score, stand.
#include <iostream>
#include <string>

#include "search.hpp"

using namespace regicide;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

}  // namespace

int main() {
    SearchResult r;
    r.partial = true;
    r.score = -SCORE_WIN + 7;
    check(r.sound_score() == SCORE_DRAW, "a partial loss without a completed iteration is not evidence");
    r.completed_score = 123;
    check(r.sound_score() == 123, "a partial loss falls back to the completed score");
    r.score = SCORE_DB_WIN;
    check(r.sound_score() == r.score, "a partial win stands");
    r.partial = false;
    r.score = -SCORE_DB_WIN;
    check(r.sound_score() == r.score, "a completed loss stands");
    return failures == 0 ? 0 : 1;
}
