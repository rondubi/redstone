#pragma once

#include <memory>

#include "disk/disk.hpp"
#include "sys/child.hpp"
#include "sys/command.hpp"

namespace redstone::replica {
struct child;

struct replica {
  std::unique_ptr<child> child;

  ~replica();
};
} // namespace redstone::replica