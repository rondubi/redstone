#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace redstone::hook {
struct handlers {
  virtual std::optional<int> maybe_handle(int sysno,
                                          std::span<const uint64_t, 6> args) {
    return std::nullopt;
  }

  virtual void signaled(int signal) {}
  virtual void exited(int status) {}
  virtual void stopped(int signal) {}
};

struct command {
  std::string path;
  std::vector<std::string> args;
  std::vector<std::pair<std::string, std::string>> env;
};

void run_command(const command &cmd, handlers &handlers);
} // namespace redstone::hook