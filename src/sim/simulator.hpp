#pragma once

#include <vector>

#include "machine.hpp"
#include "random/xoshiro.hpp"

namespace redstone::sim {
class simulator {
public:
private:
  random::xoshiro256_star_star rng_;
  std::vector<machine> machines_;
};
} // namespace redstone::sim