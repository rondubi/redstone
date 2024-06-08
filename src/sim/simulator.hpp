#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

#include "machine.hpp"
#include "net/fault.hpp"
#include "random/splitmix.hpp"
#include "random/xoshiro.hpp"

namespace redstone::sim {
struct options {
  net::net_fault_options net_faults{};
  double time_scale = 1.0;
  std::uint64_t seed;
};

class simulator {
public:
  explicit simulator(options &&o) : options_{std::move(o)} {
    random::split_mix seed_source{options_.seed};
    rng_.seed_from(seed_source);
  }

  net::network &net() { return net_; }

  random::xoshiro256_star_star &rng() { return rng_; }

  const options &initial_options() const { return options_; }

private:
  random::xoshiro256_star_star rng_;
  std::vector<machine> machines_;
  net::network net_;
  options options_;
};
} // namespace redstone::sim