#pragma once

#include "file_descriptor.hpp"
#include "runner.hpp"
#include <cassert>
#include <memory>

namespace redstone::sim {
class replica {
public:
  ~replica();

  explicit replica(std::shared_ptr<runner_handle> handle, machine *m);

  file_descriptor_table &fd_table() { return fd_table_; }

  runner_handle &runner() {
    assert(handle_);
    return *handle_;
  }

private:
  std::shared_ptr<runner_handle> handle_;
  machine *machine_ = nullptr;
  file_descriptor_table fd_table_;
};
} // namespace redstone::sim