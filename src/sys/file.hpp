#pragma once

#include <cstdint>
#include <span>
#include <unistd.h>
#include <utility>

namespace redstone::sys {
class file {
public:
  ~file() { reset(); }

  file() = default;

  explicit file(int fd) : fd_{fd} {}

  file(file &&f) : fd_{f.release()} {}

  file(const file &) = delete;

  file &operator=(file &&f) {
    std::swap(fd_, f.fd_);
    return *this;
  }

  file &operator=(const file &f) = delete;

  explicit operator bool() const { return 0 <= fd_; }

  int get() const { return fd_; }

  int release() {
    auto fd = fd_;
    fd_ = -1;
    return fd;
  }

  void reset() {
    if (*this) {
      ::close(release());
    }
  }

  std::int64_t read_at(std::uint64_t pos, std::span<std::byte> buf);

  std::int64_t write_at(std::uint64_t pos, std::span<const std::byte> buf);

  std::int64_t read_exact_at(std::uint64_t pos, std::span<std::byte> buf);
  std::int64_t write_all_at(std::uint64_t pos, std::span<const std::byte> buf);

private:
  int fd_ = -1;
};

file open(const char *path, int mode, int prot);
} // namespace redstone::sys