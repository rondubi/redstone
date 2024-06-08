#pragma once

#include <cerrno>
#include <cstddef>
#include <memory>
#include <mutex>
#include <span>
#include <tl/function_ref.hpp>
#include <unordered_map>
#include <vector>

namespace redstone::sim {
class file_descriptor {
public:
  virtual ~file_descriptor() = default;

  virtual std::int64_t
  read(tl::function_ref<int(std::span<const std::byte>)> store,
       std::size_t bytes) {
    return -EINVAL;
  }

  virtual std::int64_t write(tl::function_ref<int(std::span<std::byte>)> load,
                             std::size_t bytes) {
    return -EINVAL;
  };

private:
};

class file_descriptor_table {
public:
  file_descriptor_table() = default;

  static bool is_simulated(int fd) { return (fd & simulated) != 0; }

  std::shared_ptr<file_descriptor> get(int fd);
  int close(int fd);
  int insert(std::shared_ptr<file_descriptor> fildes);

private:
  static constexpr int simulated = 1 << 30;
  static constexpr int capacity = (1u << 30) - 1;

  int counter_ = 1 << 30;

  std::mutex mutex_;
  std::unordered_map<int, std::shared_ptr<file_descriptor>> table_;
  std::vector<int> unused_;
};

std::shared_ptr<file_descriptor> stdout_fd();
} // namespace redstone::sim