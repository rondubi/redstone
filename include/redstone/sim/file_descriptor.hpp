#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace redstone::sim {
class file_descriptor {
public:
private:
};

class file_descriptor_table {
public:
  file_descriptor_table() = default;

  bool is_simulated(int fd) const { return (fd & simulated) != 0; }

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
} // namespace redstone::sim