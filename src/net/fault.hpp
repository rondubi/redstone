#pragma once

#include <chrono>
#include <cstdio>
#include <random>

#include "random/rng.hpp"

namespace redstone::net {
struct net_fault_options {
  std::chrono::nanoseconds min_latency{0};
  std::chrono::nanoseconds max_latency{0};
  double p_drop = 0.0;
  double p_replay = 0.0;

  template <typename Rng>
  std::chrono::nanoseconds latency(Rng &&rng) const {
    return random::gen_range(rng, min_latency,
                             max_latency + std::chrono::nanoseconds{1});
  }

  template <typename Rng>
  bool should_drop(Rng &&rng) const {
    std::bernoulli_distribution dis{p_drop};
    return dis(rng);
  }

  template <typename Rng>
  std::size_t replay_count(Rng &&rng) const {
    std::size_t n = 0;
    for (;;) {
      if (!random::gen_bool(rng, p_replay))
        return n;
      n++;
    }
  }
};
} // namespace redstone::net