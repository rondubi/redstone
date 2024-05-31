#pragma once

#include <memory>

#include "redstone/disk/disk.hpp"
#include "redstone/sys/child.hpp"
#include "redstone/sys/command.hpp"

namespace redstone::replica {
struct child;

struct replica {
  std::unique_ptr<child> child;

  ~replica();
};
} // namespace redstone::replica