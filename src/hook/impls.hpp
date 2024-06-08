#pragma once

#include "hook.hpp"
#include <span>

namespace redstone::hook {
hook_result sys_write(sim::replica &replica,
                      std::span<const std::uint64_t, 6> args);
hook_result sys_read(sim::replica &replica,
                     std::span<const std::uint64_t, 6> args);
hook_result sys_close(sim::replica &replica,
                      std::span<const std::uint64_t, 6> args);
hook_result sys_open(sim::replica &replica,
                     std::span<const std::uint64_t, 6> args);
hook_result sys_socket(sim::replica &replica,
                       std::span<const std::uint64_t, 6> args);
hook_result sys_sendto(sim::replica &replica,
                       std::span<const std::uint64_t, 6> args);
hook_result sys_recvfrom(sim::replica &replica,
                         std::span<const std::uint64_t, 6> args);
hook_result sys_connect(sim::replica &replica,
                        std::span<const std::uint64_t, 6> args);
hook_result sys_read(sim::replica &replica,
                     std::span<const std::uint64_t, 6> args);
hook_result sys_bind(sim::replica &replica,
                     std::span<const std::uint64_t, 6> args);
hook_result sys_clock_gettime(sim::replica &replica,
                              std::span<const std::uint64_t, 6> args);
hook_result sys_clock_nanosleep(sim::replica &replica,
                                std::span<const std::uint64_t, 6> args);
} // namespace redstone::hook