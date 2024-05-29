#pragma once

#include <cstddef>

namespace redstone::random {
template <typename Rng>
struct rng_traits {
  static constexpr size_t seed_size = Rng::seed_size;
};

template <typename Rng, typename Source>
Rng seed_from_rng(Source &source) {
  std::byte seed[rng_traits<Rng>::seed_size];
  source.fill_bytes(seed);
  return Rng{seed};
}
} // namespace redstone::random