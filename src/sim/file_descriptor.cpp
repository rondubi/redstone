#include "redstone/sim/file_descriptor.hpp"

#include <cassert>

namespace redstone::sim {
std::shared_ptr<file_descriptor> file_descriptor_table::get(int fd) {
  std::unique_lock lock{mutex_};
  return table_[fd];
}

int file_descriptor_table::close(int fd) {
  std::unique_lock lock{mutex_};
  auto it = table_.find(fd);
  if (it == table_.end() || !it->second) {
    return -EBADFD;
  }

  assert(is_simulated(fd));
  assert(0 <= fd);

  table_.erase(it);
  unused_.push_back(fd);
  return 0;
}

int file_descriptor_table::insert(std::shared_ptr<file_descriptor> fildes) {
  std::unique_lock lock{mutex_};

  int fd = -1;

  if (!unused_.empty()) {
    fd = unused_.back();
    unused_.pop_back();
  } else {
    if (capacity <= table_.size()) {
      return -1;
    }

    fd = static_cast<int>(table_.size());

    // Simulated bit already set, this should not be a valid fd
    assert(!is_simulated(fd));
    fd |= simulated;
  }

  assert(0 <= fd);
  assert(is_simulated(fd));

  auto &slot = table_[fd];
  assert(!slot);
  slot = std::move(fildes);
  return fd;
}
} // namespace redstone::sim