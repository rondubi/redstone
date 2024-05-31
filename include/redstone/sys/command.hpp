#pragma once

#include "redstone/sys/child.hpp"

#include <string>
#include <utility>
#include <vector>

namespace redstone::sys {
struct command {
  std::string path;
  std::vector<std::string> args;
  std::vector<std::pair<std::string, std::string>> env;

  [[nodiscard]] child spawn();
};
} // namespace redstone::sys