#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

namespace regicide::egdb::detail {

// Runs body(item, thread_id) for item in [0, n) on up to `threads` threads, handing out items
// dynamically. Runs inline when one thread suffices.
template <class F>
void parallel_for(std::uint64_t n, int threads, F&& body) {
    const int t = int(std::min<std::uint64_t>(std::max(threads, 1), n));
    if (t <= 1) {
        for (std::uint64_t i = 0; i < n; ++i) body(i, 0);
        return;
    }
    std::atomic<std::uint64_t> next{0};
    auto worker = [&](int tid) {
        for (std::uint64_t i; (i = next.fetch_add(1, std::memory_order_relaxed)) < n;) body(i, tid);
    };
    std::vector<std::jthread> pool;
    pool.reserve(t - 1);
    for (int i = 1; i < t; ++i) pool.emplace_back(worker, i);
    worker(0);
}

}  // namespace regicide::egdb::detail
