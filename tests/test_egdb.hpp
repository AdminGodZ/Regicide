#pragma once
// A small endgame database for tests: 2..N pieces, WLD and MTC, built with the library's own builder into a fresh
// temporary directory and loaded. The builder logs every slice to stdout; that output is kept out of the test's.
#include <fcntl.h>

#include <cstdio>
#include <filesystem>
#include <random>
#include <string>
#include <system_error>

#include "egdb/build.hpp"
#include "egdb/egdb.hpp"

#ifdef _WIN32
#include <io.h>
#define dup _dup
#define dup2 _dup2
#define open _open
#define close _close
#else
#include <unistd.h>
#endif

namespace test_egdb {

// Sends stdout to the null device for the length of a scope.
struct QuietStdout {
    int saved;
    QuietStdout() {
        std::fflush(stdout);
        saved = dup(1);
#ifdef _WIN32
        const int null = open("NUL", O_WRONLY);
#else
        const int null = open("/dev/null", O_WRONLY);
#endif
        dup2(null, 1);
        close(null);
    }
    ~QuietStdout() {
        std::fflush(stdout);
        dup2(saved, 1);
        close(saved);
    }
};

// Builds 2..max_pieces pieces into a new directory named after `name` and loads it (only one database can be loaded
// per process). Returns the directory, or "" if the build or the load failed.
inline std::string build(const std::string& name, int max_pieces, int threads = 1) {
    const std::string dir =
        (std::filesystem::temp_directory_path() / ("regicide-" + name + "-" + std::to_string(std::random_device{}())))
            .string();
    regicide::egdb::detail::BuildOptions options;
    options.dir = dir;
    options.threads = threads;
    options.mtc_pieces = max_pieces;
    options.verbose = false;
    bool built;
    {
        QuietStdout quiet;
        built = regicide::egdb::detail::build_range(options, 2, max_pieces);
    }
    if (built && regicide::egdb::init(dir, 64) && regicide::egdb::max_pieces() >= max_pieces) return dir;
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    return "";
}

// Best effort: mapped files cannot be removed on Windows.
inline void remove(const std::string& dir) {
    std::error_code ec;
    if (!dir.empty()) std::filesystem::remove_all(dir, ec);
}

}  // namespace test_egdb

#ifdef _WIN32
#undef dup
#undef dup2
#undef open
#undef close
#endif
