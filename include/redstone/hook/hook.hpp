#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace redstone {
class simulator;
class instance;
} // namespace redstone

namespace redstone::hook {
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

struct handlers {
  virtual std::optional<std::uint64_t>
  maybe_handle(std::uint64_t sysno, std::span<const uint64_t, 6> args) {
    return std::nullopt;
  }

  virtual void signaled(int signal) {}
  virtual void exited(int status) {}
  virtual void stopped(int signal) {}
};

struct command {
  std::string path;
  std::vector<std::string> args;
  std::vector<std::string> env;
};

void run_command(const command &cmd, handlers &handlers);

void print_stats();
} // namespace redstone::hook