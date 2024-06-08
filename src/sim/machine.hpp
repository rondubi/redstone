#pragma once

#include <functional>
#include <memory>

#include "replica.hpp"
#include "runner.hpp"
#include "simulator.hpp"

namespace redstone::sim {
class simulator;

class machine {
public:
  explicit machine(simulator &sim, runner_options options)
      : sim_{&sim}, runner_options_{std::move(options)} {}

  std::shared_ptr<replica> current_replica() { return current_; }

  void start();

  sim::simulator &sim() { return *sim_; }

private:
  runner_options runner_options_;
  std::shared_ptr<replica> current_;
  std::function<std::shared_ptr<runner_handle>(const runner_options &)> runner_{
      ptrace_run};
  sim::simulator *sim_;
  net::network *net_;
};
} // namespace redstone::sim