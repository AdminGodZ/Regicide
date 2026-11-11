// Re-searches the records of a DataRecord file with the current engine and the endgame database.
//
//   rescore --egdb DIR --in IN.bin --out OUT.bin [--threads 8] [--nodes 9:50000,13:20000,17:10000]
//           [--min-pieces 9] [--keep-above 24] [--evalfile net.nnue] [--hash 32] [--recheck-losses]
//
// OUT gets one record for every record of IN, in the same order. Records with min-pieces..keep-above pieces get a new
// score from a search with the node budget of their piece count (--nodes: "from pieces:nodes" steps); a decisive
// score that label.hpp accepts as proven (a win; a loss only from a complete iteration) is clamped to
// +-DataRecord::PROVEN_SCORE and flagged (stm bit 7). Other records are copied unchanged. --recheck-losses re-searches
// only the records already flagged as proven losses (for files rescored before that rule).
// Game results are never changed (relabel them first with `truth relabel`).
//
// Resumable: OUT holds a whole-record prefix of the result. Run the same command again and it continues after the
// last complete record. SIGSTOP/SIGCONT pause it.
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

#include "datafile.hpp"
#include "egdb/egdb.hpp"
#include "label.hpp"
#include "search.hpp"

using namespace regicide;

namespace {

constexpr long CHUNK = 256;  // records per work unit (a restart repeats at most ~4 x threads units)

long file_records(const std::string& path) {
    struct stat s;
    return stat(path.c_str(), &s) == 0 ? long(s.st_size / 16) : -1;
}

}  // namespace

int main(int argc, char** argv) {
    std::string egdb_dir, in, out, evalfile;
    int threads = 8, min_pieces = 9, keep_above = 24;
    bool recheck_losses = false;  // re-search only the proven-loss records (rule change of 2026-11-10, label.hpp)
    std::size_t hash_mb = 32;
    std::vector<std::pair<int, std::uint64_t>> steps{{9, 50000}, {13, 20000}, {17, 10000}};
    for (int i = 1; i < argc; ++i) {
        const std::string k = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (k == "--egdb") egdb_dir = next();
        else if (k == "--in") in = next();
        else if (k == "--out") out = next();
        else if (k == "--threads") threads = std::stoi(next());
        else if (k == "--min-pieces") min_pieces = std::stoi(next());
        else if (k == "--keep-above") keep_above = std::stoi(next());
        else if (k == "--evalfile") evalfile = next();
        else if (k == "--hash") hash_mb = std::stoul(next());
        else if (k == "--recheck-losses") recheck_losses = true;
        else if (k == "--nodes") {
            steps.clear();
            const std::string s = next();
            for (std::size_t p = 0; p < s.size();) {
                const std::size_t c = std::min(s.find(',', p), s.size());
                const std::string item = s.substr(p, c - p);
                const std::size_t colon = item.find(':');
                steps.push_back({std::stoi(item.substr(0, colon)), std::stoull(item.substr(colon + 1))});
                p = c + 1;
            }
        } else {
            std::cerr << "unknown option " << k << '\n';
            return 2;
        }
    }
    if (egdb_dir.empty() || !egdb::init(egdb_dir, 4096)) {
        std::cerr << "cannot load egdb (--egdb DIR)\n";
        return 1;
    }
    std::shared_ptr<const Evaluator> ev = Evaluator::builtin();
    if (!evalfile.empty() && !(ev = Evaluator::from_file(evalfile))) {
        std::cerr << "cannot load " << evalfile << '\n';
        return 1;
    }
    auto budget = [&](int pieces) -> std::uint64_t {
        std::uint64_t b = 0;
        for (const auto& [from, nodes] : steps)
            if (pieces >= from) b = nodes;
        return b;
    };

    const long total = file_records(in);
    if (total < 0) {
        std::cerr << "cannot open " << in << '\n';
        return 1;
    }
    long start = std::max(0L, file_records(out));
    if (start > total) {
        std::cerr << out << " is longer than " << in << '\n';
        return 1;
    }
    if (truncate(out.c_str(), start * 16) != 0 && start > 0) {
        std::cerr << "cannot truncate " << out << '\n';
        return 1;
    }
    std::FILE* fin = std::fopen(in.c_str(), "rb");
    std::FILE* fout = std::fopen(out.c_str(), "ab");
    if (!fin || !fout) {
        std::cerr << "cannot open files\n";
        return 1;
    }
    std::printf("rescore %s -> %s: %ld records, resuming at %ld, %d threads, egdb %d pieces, pieces %d..%d searched\n",
                in.c_str(), out.c_str(), total, start, threads, egdb::max_pieces(), min_pieces, keep_above);
    std::fflush(stdout);

    std::mutex mu;
    std::condition_variable cv;
    long next_chunk = 0, written_chunks = 0;
    std::map<long, std::vector<DataRecord>> done;  // finished chunks waiting for their turn
    const long nchunks = (total - start + CHUNK - 1) / CHUNK;
    std::atomic<long> searched{0}, proven{0}, flipped{0}, untrusted{0}, partial_losses{0};
    const auto t0 = std::chrono::steady_clock::now();
    long last_report = 0;

    auto worker = [&] {
        Engine engine;
        engine.set_hash_mb(hash_mb);
        engine.set_evaluator(ev);
        engine.set_egdb_pieces(egdb::max_pieces());
        std::vector<DataRecord> buf;
        for (;;) {
            long c;
            {
                std::unique_lock lock(mu);
                // Bound the reorder buffer: don't run too far ahead of the writer.
                cv.wait(lock, [&] { return next_chunk >= nchunks || next_chunk - written_chunks < 4L * threads; });
                if (next_chunk >= nchunks) return;
                c = next_chunk++;
                const long first = start + c * CHUNK, n = std::min(CHUNK, total - first);
                buf.resize(std::size_t(n));
                std::fseek(fin, first * 16, SEEK_SET);
                if (std::fread(buf.data(), 16, buf.size(), fin) != buf.size()) {
                    // Not std::exit: it would destroy the database tables under the other workers' searches.
                    // What is on disk stays; a rerun truncates a partial record and continues from there.
                    std::cerr << "read error\n";
                    std::_Exit(1);
                }
            }
            engine.new_game();
            for (DataRecord& r : buf) {
                const int pieces = r.pieces();
                if (recheck_losses ? !(r.proven() && r.score < 0) : (pieces < min_pieces || pieces > keep_above || r.proven()))
                    continue;
                Limits l;
                l.nodes = budget(pieces);
                l.use_book = false;
                const Position pos = r.position();
                engine.set_position(pos);
                // label_of (label.hpp): wins are proofs, losses only from a complete iteration; a search that
                // completed nothing leaves the record as it was.
                const auto lab = label_of(engine.go(l));
                if (!lab) {
                    untrusted++;
                    continue;
                }
                if (lab->partial_loss && !lab->proven) partial_losses++;
                const int s = lab->score;
                const std::int8_t result = r.result;
                const std::uint8_t start_flag = r.stm & DataRecord::GAME_START;
                const int old = r.score;
                r = lab->proven ? DataRecord::make_proven(pos, s) : DataRecord::make(pos, s);
                r.result = result;
                r.stm |= start_flag;
                searched++;
                if (r.proven()) proven++;
                if ((old > 0 && r.score < 0) || (old < 0 && r.score > 0)) flipped++;
            }
            std::unique_lock lock(mu);
            done[c] = std::move(buf);
            buf = {};
            while (!done.empty() && done.begin()->first == written_chunks) {
                const auto& v = done.begin()->second;
                std::fwrite(v.data(), 16, v.size(), fout);
                done.erase(done.begin());
                ++written_chunks;
            }
            std::fflush(fout);
            cv.notify_all();
            const long w = start + std::min(total - start, written_chunks * CHUNK);
            if (w - last_report >= 200000 || written_chunks == nchunks) {
                last_report = w;
                const double sec =
                    std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                std::printf("written %ld / %ld (%.2f%%)  searched %ld  proven %ld  sign flips %ld  untrusted %ld  partial losses %ld  %.0f searched/s\n",
                            w, total, 100.0 * w / total, searched.load(), proven.load(), flipped.load(), untrusted.load(), partial_losses.load(),
                            searched / std::max(1e-9, sec));
                std::fflush(stdout);
            }
        }
    };
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t) pool.emplace_back(worker);
    for (auto& t : pool) t.join();
    std::fclose(fout);
    std::fclose(fin);
    const long n = file_records(out);
    std::printf("done: %s has %ld of %ld records\n", out.c_str(), n, total);
    return n == total ? 0 : 1;
}
