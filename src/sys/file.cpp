#include "file.hpp"
#include "child.hpp"
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <fmt/core.h>
#include <system_error>
#include <unistd.h>

namespace redstone::sys {
std::uint64_t file::lseek(std::uint64_t pos) {
  auto res = ::lseek(get(), pos, SEEK_SET);
  if (res < 0) {
    throw std::system_error{errno, std::generic_category(), "lseek failed"};
  }
  return res;
}

std::int64_t file::read(std::span<std::byte> buf) {
  auto res = ::read(fd_, buf.data(), buf.size_bytes());
  if (res < 0) {
    assert(0 < errno);
    return -errno;
  }
  return res;
}

int file::read_exact(std::span<std::byte> buf) {
  while (!buf.empty()) {
    assert(buf.size_bytes() != 0);
    auto res = read(buf);
    if (res < 0) {
      if (-res == EINTR || -res == EAGAIN) {
        continue;
      }
      return res;
    }
    if (res == 0) {
      fmt::println("EOF");
      return -EINVAL;
    }

    buf = buf.subspan(res);
  }
  return 0;
}

std::int64_t file::read_at(std::uint64_t pos, std::span<std::byte> buf) {
  auto off = static_cast<off_t>(pos);
  if (off < 0)
    return -EINVAL;
  auto res = ::pread(fd_, buf.data(), buf.size_bytes(), off);
  if (res < 0) {
    assert(0 < errno);
    return -errno;
  }
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
    assert(buf.size_bytes() != 0);
    auto res = read_at(pos, buf);
    if (res < 0) {
      if (-res == EINTR || -res == EAGAIN) {
        continue;
      }
      return res;
    }
    assert(res != 0);

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

file proc_mem(sys::child &child, int mode) {
  auto path = fmt::format("/proc/{}/mem", child.pid());
  auto res = ::open(path.c_str(), mode, 0666);
  if (res < 0) {
    throw std::system_error{errno, std::generic_category(),
                            fmt::format("open {} failed", path)};
  }
  auto f = file{res};
  assert(f);
  return f;
}
} // namespace redstone::sys