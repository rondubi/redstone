#pragma once

#include <functional>
#include <memory>

#include "replica.hpp"
#include "runner.hpp"

namespace redstone::sim {
class machine {
public:
  explicit machine(runner_options options)
      : runner_options_{std::move(options)} {}

  std::shared_ptr<replica> current_replica() { return current_; }

  void start();

private:
  runner_options runner_options_;
  std::shared_ptr<replica> current_;
  std::function<std::shared_ptr<runner_handle>(const runner_options &)> runner_{
      ptrace_run};
};
} // namespace redstone::sim