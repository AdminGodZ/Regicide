#!/usr/bin/env python3
"""WP9: tools/match: an openings-file line "FEN | moves | A=B" (or A=W) fixes engine A's colour for that opening;
the game with the other colour is not scheduled. Lines without the third field behave as before."""
import sys

root = sys.argv[1]


def patch(path, pairs):
    s = open(path).read()
    for old, new in pairs:
        if s.count(old) != 1:
            sys.exit(f"{path}: expected one occurrence of:\n{old}")
        s = s.replace(old, new)
    open(path, "w").write(s)


patch(f"{root}/tools/match/openings.hpp", [
    ("""    std::vector<cbt::Move> played;  // `moves`, parsed
};""", """    std::vector<cbt::Move> played;  // `moves`, parsed
    char a_colour = 0;            // 'B' or 'W': engine A plays only this colour here (file field "A=B"/"A=W")
};"""),
    ("""//    position ("9-13 22-18 6-9"), or "FEN | moves"; '#' starts a comment;""",
     """//    position ("9-13 22-18 6-9"), or "FEN | moves", or "FEN | moves | A=B" (or A=W: engine A plays only that
//    colour in this opening; the game with colours reversed is not played); '#' starts a comment;"""),
])
patch(f"{root}/tools/match/openings.cpp", [
    ("""        if (bar != std::string::npos) {
            o.fen = trim(line.substr(0, bar));
            o.moves = trim(line.substr(bar + 1));""",
     """        if (bar != std::string::npos) {
            o.fen = trim(line.substr(0, bar));
            o.moves = trim(line.substr(bar + 1));
            if (auto bar2 = o.moves.find('|'); bar2 != std::string::npos) {
                const std::string side = trim(o.moves.substr(bar2 + 1));
                o.moves = trim(o.moves.substr(0, bar2));
                if (side == "A=B" || side == "A=W") {
                    o.a_colour = side[2];
                } else {
                    err = deck + ":" + std::to_string(lineno) + ": third field must be A=B or A=W";
                    return {};
                }
            }"""),
])
patch(f"{root}/tools/match/main.cpp", [
    ("""    std::optional<long> take_task() {
        std::lock_guard lock(mu);
        if (stop_scheduling || g_interrupted || next_task >= total_games) return std::nullopt;""",
     """    // Openings with a fixed colour for A (file field A=B / A=W) play only that leg (0: A black, 1: A white).
    bool skipped(long task) const {
        const char c = opening_for(task).a_colour;
        return c && (task % 2) != (c == 'B' ? 0 : 1);
    }

    std::optional<long> take_task() {
        std::lock_guard lock(mu);
        while (next_task < total_games && skipped(next_task)) ++next_task;
        if (stop_scheduling || g_interrupted || next_task >= total_games) return std::nullopt;"""),
])
print("patched tools/match (A=B/A=W opening field)")
