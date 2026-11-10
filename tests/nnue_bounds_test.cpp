// Integer safety of the NNUE: the output sum is computed in 64 bits and clamped instead of wrapping, and the loader
// rejects nets whose biases leave no headroom in the int32 hidden sums or whose output scale could overflow.
#include <climits>
#include <cstdint>
#include <iostream>
#include <string>

#include "nnue.hpp"
#include "position.hpp"
#include "test_net.hpp"

using namespace regicide;
using test_net::put;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

bool loads(const std::vector<unsigned char>& d) { return nnue::Network::load(d.data(), d.size()) != nullptr; }

}  // namespace

int main() {
    // The largest output bias plus one active L2 unit: the int32 sum would wrap to a large loss.
    auto wide = test_net::bytes();
    put<std::int32_t>(wide, test_net::L2_BIAS, 8128);  // L2 unit 0 at its 127 maximum
    put<std::int32_t>(wide, test_net::OUT_BIAS, INT32_MAX);
    put<std::int16_t>(wide, test_net::OUT_WEIGHT, 1);
    const auto net = nnue::Network::load(wide.data(), wide.size());
    check(net != nullptr, "the wide-output net loads");
    if (net) check(net->evaluate(Position()) == SCORE_KNOWN_WIN - 1, "the output sum is clamped, not wrapped");

    const auto base = test_net::bytes();
    auto bad = base;
    put<std::int32_t>(bad, test_net::L1_BIAS, INT32_MAX);
    check(!loads(bad), "an L1 bias without headroom is rejected");
    bad = base;
    put<std::int32_t>(bad, test_net::L2_BIAS, INT32_MIN);
    check(!loads(bad), "an L2 bias without headroom is rejected");
    bad = base;
    put<std::uint32_t>(bad, test_net::OUT_SCALE, (1u << 24) + 1);
    check(!loads(bad), "an output scale above 2^24 is rejected");
    auto max_scale = base;
    put<std::uint32_t>(max_scale, test_net::OUT_SCALE, 1u << 24);
    check(loads(max_scale), "the largest supported output scale loads");
    return failures == 0 ? 0 : 1;
}
