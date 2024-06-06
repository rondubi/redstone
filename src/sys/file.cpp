#include "file.hpp"
#include <cerrno>
#include <unistd.h>

namespace redstone::sys {
std::int64_t file::read_at(std::uint64_t pos, std::span<std::byte> buf) {
  auto off = static_cast<off_t>(pos);
  if (off < 0)
    return -EINVAL;
  auto res = ::pread(fd_, buf.data(), buf.size_bytes(), off);
  if (res < 0)
    return -errno;
  return res;
}

std::int64_t file::write_at(std::uint64_t pos, std::span<const std::byte> buf) {
  auto off = static_cast<off_t>(pos);
  if (off < 0)
    return -EINVAL;
  auto res = ::pwrite(fd_, buf.data(), buf.size_bytes(), off);
  if (res < 0)
    return -errno;
  return res;
}

std::int64_t file::read_exact_at(std::uint64_t pos, std::span<std::byte> buf) {
  while (!buf.empty()) {
    auto res = read_at(pos, buf);
    if (res < 0) {
      if (-res == EINTR || -res == EAGAIN) {
        continue;
      }
      return res;
    }

    buf = buf.subspan(res);
    pos += res;
  }
  return 0;
}

std::int64_t file::write_all_at(std::uint64_t pos,
                                std::span<const std::byte> buf) {
  while (!buf.empty()) {
    auto res = write_at(pos, buf);
    if (res < 0) {
      if (-res == EINTR || -res == EAGAIN) {
        continue;
      }
      return res;
    }

    buf = buf.subspan(res);
    pos += res;
  }
  return 0;
}
} // namespace redstone::sys