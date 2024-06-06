#pragma once

#include "hook.hpp"

namespace redstone::hook {
hook_result sys_write(sim::replica &replica,
                      std::span<const std::uint64_t, 6> args);
hook_result sys_read(sim::replica &replica,
                     std::span<const std::uint64_t, 6> args);
hook_result sys_close(sim::replica &replica,
                      std::span<const std::uint64_t, 6> args);
} // namespace redstone::hook