#pragma once

#include "file_descriptor.hpp"
#include "net/network.hpp"
#include "runner.hpp"

#include <cassert>
#include <chrono>
#include <memory>

namespace redstone::sim {
using clock = std::chrono::steady_clock;

class simulator;

class replica {
public:
  ~replica();

  explicit replica(std::shared_ptr<runner_handle> handle, machine &m);

  file_descriptor_table &fd_table() { return fd_table_; }

  runner_handle &runner() {
    assert(handle_);
    return *handle_;
  }

  net::network &network();

  simulator &sim();

  clock::time_point epoch() const { return epoch_; }

private:
  std::shared_ptr<runner_handle> handle_;
  machine *machine_ = nullptr;
  net::network *net_;
  file_descriptor_table fd_table_;
  const clock::time_point epoch_ = clock::now();
};
} // namespace redstone::sim