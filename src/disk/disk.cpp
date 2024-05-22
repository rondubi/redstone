#include "redstone/disk/disk.hpp"
#include <_types/_uint64_t.h>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sys/_types/_int64_t.h>
#include <sys/_types/_seek_set.h>
#include <utility>
#include <vector>

namespace redstone::disk {

// disk_file functions

int64_t disk_file::write(std::uint64_t pos, std::span<const std::byte> buf) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (pos < 0)
    return -22; // Invalid arg
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
    return -22; // Invalid arg

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
  size_t bytes_written = pwrite_unlocked(buf, pos_);
  pos_ += bytes_written;
  return bytes_written;
}

int64_t open_file::read(std::span<std::byte> buf) {
  std::lock_guard<std::mutex> lock(mutex_);
  size_t bytes_read = pread_unlocked(buf, pos_);
  pos_ += bytes_read;
  return bytes_read;
}

int64_t open_file::pwrite_unlocked(std::span<const std::byte> buf,
                                   size_t offset) {
  if (direct_) {
    size_t bytes_written = disk_->write(offset, buf);
  } else {
    write_backlog_.push_back({{buf.begin(), buf.end()}, offset});
  }
  return buf.size();
}

int64_t open_file::pread_unlocked(std::span<std::byte> buf, size_t offset) {
  flush_backlog();
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

void open_file::flush_backlog() {
  if (direct_) {
    return;
  }

  for (const write_req &req : write_backlog_) {
    disk_->write(req.pos, req.bytes);
  }
  write_backlog_.clear();
}

} // namespace redstone::disk
