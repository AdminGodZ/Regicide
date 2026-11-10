// datagen labels a cut-short losing search as unproven, with the last completed iteration's score: a partial
// iteration that has only seen losing lines must not drop the record or turn it into a proven loss.
//
// B:WK5,K22,K25,29:BK10 with a net that scores every position +100 for the side to move. datagen's own sequence is
// repeated first: a 2000-node screening search (must be within its 300 opening limit), then the 6144-node labelling
// search, which must end in a partial iteration with a losing score (-29986) and a sound score of -100. The test
// checks that the fixture still behaves so (a search change may need a new fixture), then runs datagen for one game
// seeded with the position: its first record must be the seed, scored -100, not proven, with the game-start flag.
//
//   datagen_partial_loss_test DATAGEN    the datagen executable
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "datafile.hpp"
#include "engine_process.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"
#include "test_net.hpp"

using namespace regicide;

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: datagen_partial_loss_test DATAGEN\n";
        return 2;
    }
    const auto dir =
        std::filesystem::temp_directory_path() /
        ("regicide-datagen-label-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(dir);
    auto net = test_net::bytes();
    test_net::put<std::int32_t>(net, test_net::OUT_BIAS, 2032);  // +100 for the side to move
    {
        std::ofstream file(dir / "net.nnue", std::ios::binary);
        file.write(reinterpret_cast<const char*>(net.data()), std::streamsize(net.size()));
    }

    Position root;
    root.set_fen("B:WK5,K22,K25,29:BK10");
    Engine engine;
    engine.set_hash_mb(16);
    engine.set_evaluator(Evaluator::from_file((dir / "net.nnue").string()));
    engine.new_game();
    engine.set_position(root);
    Limits limits;
    limits.nodes = 2000;
    const SearchResult screen = engine.go(limits);
    engine.set_position(root);
    limits.nodes = 6144;
    const SearchResult result = engine.go(limits);
    if (std::abs(screen.sound_score()) > 300 || !result.partial || result.score > -SCORE_KNOWN_WIN ||
        is_decisive(result.sound_score())) {
        std::cerr << "FAIL: the partial-loss fixture changed: screening " << screen.sound_score() << ", raw "
                  << result.score << ", sound " << result.sound_score() << ", partial " << result.partial << '\n';
        std::filesystem::remove_all(dir);
        return 2;
    }

    {
        std::ofstream file(dir / "seed.txt");
        file << root.fen() << '\n';
    }
    match::Process datagen;
    std::string error;
    bool ok = datagen.start(
        {argv[1], "--games", "1", "--threads", "1", "--nodes", "6144", "--evalfile", (dir / "net.nnue").string(),
         "--seeds", (dir / "seed.txt").string(), "--seed-plies", "0", "--out", (dir / "records.bin").string()},
        "/dev/null", error);
    std::string line;
    const auto deadline = match::Clock::now() + std::chrono::seconds(30);
    if (ok)
        while (datagen.read_line(line, deadline) == match::Process::Read::Line) {
        }
    datagen.kill();

    DataRecord first{};
    std::ifstream records(dir / "records.bin", std::ios::binary);
    ok = ok && bool(records.read(reinterpret_cast<char*>(&first), sizeof(first))) && first.position() == root &&
         first.score == result.sound_score() && !first.proven() && first.game_start();
    if (!ok)
        std::cerr << "FAIL: datagen lost the partial-loss record: expected " << root.fen() << " scored "
                  << result.sound_score() << " (raw " << result.score << "), unproven, game start " << error << '\n';
    std::filesystem::remove_all(dir);
    return ok ? 0 : 1;
}
