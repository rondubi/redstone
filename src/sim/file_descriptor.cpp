#include "sim/file_descriptor.hpp"
#include "sys/file.hpp"

#include <cassert>
#include <cstddef>
#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

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

namespace {
class stdout_file_descriptor final : public file_descriptor {
public:
  stdout_file_descriptor() = default;

  std::int64_t write(tl::function_ref<int(std::span<std::byte>)> load,
                     const std::size_t bytes) final {
    std::vector buf{8192, std::byte{}};

    std::size_t done = 0;

    while (done < bytes) {
      std::span tmp = buf;

      if (tmp.size_bytes() > bytes) {
        tmp = tmp.subspan(0, bytes);
      }

      spdlog::trace("f1");
      auto res = load(tmp);
      spdlog::trace("f2");
      if (res < 0) {
        return res;
      }

      std::fwrite(tmp.data(), 1, tmp.size_bytes(), stdout);

      done += tmp.size_bytes();
    }

    return done;
  };

private:
  sys::file f_;
};
} // namespace

std::shared_ptr<file_descriptor> stdout_fd() {
  static std::shared_ptr<stdout_file_descriptor> shared =
      std::make_shared<stdout_file_descriptor>();
  return shared;
}
} // namespace redstone::sim