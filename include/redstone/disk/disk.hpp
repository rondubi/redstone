#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>
#include <sys/_types/_ssize_t.h>
#include <unordered_map>
#include <vector>

namespace redstone::disk {
class file {
public:
  size_t read(std::uint64_t pos, std::span<std::byte> buf);
  size_t write(std::uint64_t pos, std::span<const std::byte> buf);
  int saveBuffer();
  void setState(bool state);

private:
  std::mutex mutex_;
  std::vector<std::byte> buffer_;
  bool open_;
};
} // namespace redstone::disk