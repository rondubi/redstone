#pragma once

#include <cstdint>
#include <span>

#include "redstone.hpp"

namespace redstone {
enum class hook_kind {
  explicit_passthrough,
};

enum class hook_result_kind {
  handled,
  passthrough,
};

struct hook_result {
  hook_result_kind kind;
  union {
    std::uint64_t handled;
  };
};

using hook = hook_result (*)(simulator &sim, instance &instance,
                             std::span<const std::uint64_t, 6> args);

std::span<const hook> hook_table();
hook get_hook(uint64_t sys);
} // namespace redstone