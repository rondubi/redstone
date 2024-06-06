#include "disk/disk.hpp"

#include <sys/types.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <vector>

namespace redstone::disk {
constexpr auto max_file_size =
    static_cast<uint64_t>(std::numeric_limits<int64_t>::max());

// disk_file functions

int64_t disk_file::write(std::uint64_t pos, std::span<const std::byte> buf) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (pos < 0 || max_file_size <= pos)
    return -EINVAL; // Invalid arg

  // Expand buffer if more space is needed
  if (pos + buf.size_bytes() > buffer_.size()) {
    // Calculate the new size needed
    size_t newSize = pos + buf.size_bytes();

    // Resize the buffer
    buffer_.resize(newSize);
  }
  // Copy the data from 'buf' into 'buffer_' at the specified position 'pos'
  std::copy(buf.begin(), buf.end(), buffer_.begin() + pos);

  // Return the number of bytes written (which is the size of 'buf')
  return buf.size_bytes();
}

int64_t disk_file::read(std::uint64_t pos, std::span<std::byte> buf) {
  std::lock_guard<std::mutex> lock(mutex_);

  // Calculate the number of bytes available to read starting from 'pos'
  size_t availableBytes = buffer_.size() - pos;

  if (pos < 0 || pos >= buffer_.size())
    return -EINVAL; // Invalid arg

  // Copy the data from 'buffer_' into 'buf' starting at the specified position
  // 'pos'
  size_t num_elements_to_copy =
      std::min(static_cast<std::uint64_t>(buf.size()), buffer_.size() - pos);

  // Perform the copy
  std::copy_n(buffer_.begin() + pos, num_elements_to_copy, buf.begin());

  // Return the number of bytes read
  return num_elements_to_copy;
}

size_t disk_file::get_size() { return buffer_.size(); }

// open_file functions

int64_t open_file::write(std::span<const std::byte> buf) {
  std::lock_guard<std::mutex> lock(mutex_);
  int64_t result = pwrite_unlocked(buf, pos_);
  if (result < 0) {
    return result;
  }
  pos_ += static_cast<size_t>(result);
  return result;
}

int64_t open_file::read(std::span<std::byte> buf) {
  std::lock_guard<std::mutex> lock(mutex_);
  int64_t result = pread_unlocked(buf, pos_);
  if (result < 0) {
    return result;
  }
  pos_ += static_cast<size_t>(result);
  return result;
}

int64_t open_file::pwrite_unlocked(std::span<const std::byte> buf,
                                   size_t offset) {
  if (direct_) {
    size_t bytes_written = disk_->write(offset, buf);
    return static_cast<int64_t>(bytes_written);
  } else {
    write_backlog_.push_back({{buf.begin(), buf.end()}, offset});
    return buf.size();
  }
}

int64_t open_file::pread_unlocked(std::span<std::byte> buf, size_t offset) {
  fsync_unlocked();
  size_t bytes_read = disk_->read(offset, buf);
  return bytes_read;
}

int64_t open_file::pwrite(std::span<const std::byte> buf, size_t offset) {
  std::lock_guard<std::mutex> lock(mutex_);
  return pwrite_unlocked(buf, offset);
}

int64_t open_file::pread(std::span<std::byte> buf, size_t offset) {
  std::lock_guard<std::mutex> lock(mutex_);
  return pread_unlocked(buf, offset);
}

int64_t open_file::lseek(size_t offset, int whence) {
  std::lock_guard<std::mutex> lock(mutex_);
  switch (whence) {
  case SEEK_SET:
    pos_ = offset;
    break;
  case SEEK_CUR:
    pos_ += offset;
    break;
  case SEEK_END:
    pos_ = disk_->get_size() + offset;
    break;
  default:
    return -EINVAL;
  }
  return pos_;
}

int open_file::fsync() {
  std::lock_guard lock{mutex_};
  return fsync_unlocked();
}

int open_file::fsync_unlocked() {
  if (direct_) {
    return 0;
  }

  for (const write_req &req : write_backlog_) {
    disk_->write(req.pos, req.bytes);
  }
  write_backlog_.clear();
  return 0;
}
} // namespace redstone::disk
