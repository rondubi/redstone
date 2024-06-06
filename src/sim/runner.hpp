#pragma once

#include <csignal>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

#include "sys/child.hpp"

namespace redstone {
class simulator;
class instance;
} // namespace redstone

namespace redstone::sim {
class machine;

struct runner_options {
  std::string path;
  std::vector<std::string> args;
  std::vector<std::string> env;
  simulator *simulator;
  machine *machine;
};

struct runner_result {
  std::variant<sys::child::exited, sys::child::terminated> status;
};

/// Communicate with an active runner
struct runner_handle {
  virtual ~runner_handle() = default;

  virtual void kill(int signal = SIGTERM) = 0;
  virtual void wait() = 0;
  virtual std::optional<sys::child::run_state> state() = 0;
  virtual int write_memory(uintptr_t ptr, std::span<const std::byte> data) = 0;
  virtual int read_memory(uintptr_t ptr, std::span<std::byte> data) = 0;
};

std::shared_ptr<runner_handle> ptrace_run(const runner_options &options);
} // namespace redstone::sim