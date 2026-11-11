// Integer safety of the NNUE: the output sum is computed in 64 bits and clamped instead of wrapping, and the loader
// rejects nets whose biases leave no headroom in the int32 hidden sums, whose output scale could overflow, or whose
// feature-transformer sums could leave the 16-bit accumulator.
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

    // The accumulator is 16 bits wide: feature-transformer unit 0 may not be able to leave that range on a board of
    // 24 pieces. FT_BIAS and FT_WEIGHT are i16 fields; the weight of input f for unit u is at FT_WEIGHT + 2 * (128 f + u).
    constexpr std::size_t FT_BIAS = 44, FT_WEIGHT = 300;
    auto ft = base;
    put<std::int16_t>(ft, FT_BIAS, INT16_MAX);
    check(loads(ft), "the largest feature-transformer bias loads when no weight adds to it");
    put<std::int16_t>(ft, FT_WEIGHT, 1);
    check(!loads(ft), "a feature-transformer sum that can pass 32767 is rejected");
    ft = base;
    put<std::int16_t>(ft, FT_BIAS, INT16_MIN);
    put<std::int16_t>(ft, FT_WEIGHT + 2 * 128 * 5, -1);
    check(!loads(ft), "a feature-transformer sum that can pass -32768 is rejected");
    ft = base;
    for (int f = 0; f < 128; ++f) put<std::int16_t>(ft, FT_WEIGHT + 2 * 128 * f, 5 * 255);
    put<std::int16_t>(ft, FT_BIAS, 8 * 255);
    check(loads(ft), "weights and bias at the trainer's clipping limits load");
    put<std::int16_t>(ft, FT_BIAS, 8 * 255 + 128);
    check(!loads(ft), "24 pieces at the weight limit with a larger bias are rejected");
    return failures == 0 ? 0 : 1;
}
