#pragma once
// Turning a search result into a training label (rescore, datagen).
//
// A record is PROVEN only if the search proved it:
//   - a win for the side to move (score >= SCORE_KNOWN_WIN): one winning move is a proof, even if the last
//     iteration was cut short (SearchResult::partial);
//   - a loss (score <= -SCORE_KNOWN_WIN) only if the last iteration was complete: a partial iteration's loss covers
//     only the root moves searched so far. Such a "partial loss" falls back to the last completed iteration's score
//     (completed_score), which is proven only if it is decisive itself.
// A search that completed no iteration gives no label.
#include <optional>

#include "search.hpp"

namespace regicide {

struct Label {
    int score = 0;
    bool proven = false;
    bool partial_loss = false;  // the result's score was a partial loss; completed_score was used instead
};

inline std::optional<Label> label_of(const SearchResult& r) {
    auto from = [](int s, bool partial_loss) -> std::optional<Label> {
        if (s < -SCORE_WIN || s > SCORE_WIN) return std::nullopt;  // no real score (e.g. -SCORE_INF)
        return Label{s, is_decisive(s), partial_loss};
    };
    if (r.score >= SCORE_KNOWN_WIN) return from(r.score, false);
    if (r.score <= -SCORE_KNOWN_WIN && r.partial) return r.depth >= 1 ? from(r.completed_score, true) : std::nullopt;
    if (r.depth < 1 && !r.partial) return std::nullopt;  // nothing was searched
    return from(r.score, false);
}

}  // namespace regicide
