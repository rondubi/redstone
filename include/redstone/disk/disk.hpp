#pragma once

#include <sys/types.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <unordered_map>
#include <vector>

namespace redstone::disk {

class disk_file {
public:
  int64_t read(std::uint64_t pos, std::span<std::byte> buf);
  int64_t write(std::uint64_t pos, std::span<const std::byte> buf);
  size_t get_size();

private:
  std::mutex mutex_;
  std::vector<std::byte> buffer_;
};

class open_file {
  struct write_req {
    std::vector<std::byte> bytes;
    uint64_t pos;
  };

public:
  int64_t read(std::span<std::byte> buf);
  int64_t write(std::span<const std::byte> buf);
  int64_t pread(std::span<std::byte> buf, size_t offset);
  int64_t pwrite(std::span<const std::byte> buf, size_t offset);
  int64_t lseek(size_t offset, int whence);
  int fsync();

private:
  std::vector<write_req> write_backlog_;
  std::mutex mutex_;
  std::shared_ptr<disk_file> disk_;
  uint64_t pos_;
  bool direct_;

  int fsync_unlocked();
  int64_t pread_unlocked(std::span<std::byte> buf, size_t offset);
  int64_t pwrite_unlocked(std::span<const std::byte> buf, size_t offset);
};

class disk {
public:
private:
  std::unordered_map<std::string, std::shared_ptr<disk_file>> filenameMap;
};

} // namespace redstone::disk