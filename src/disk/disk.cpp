#include "redstone/disk/disk.hpp"
#include <fstream>
#include <iostream>
#include <mutex>

namespace redstone::disk {

// File Functions

size_t file::write(std::uint64_t pos, std::span<const std::byte> buf) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!this->open_)
    throw std::invalid_argument("File not open.");
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

size_t file::read(std::uint64_t pos, std::span<std::byte> buf) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!this->open_)
    throw std::invalid_argument("File not open.");

  // Calculate the number of bytes available to read starting from 'pos'
  size_t availableBytes = buffer_.size() - pos;

  // Check if the requested size exceeds the available data in the buffer
  if (buf.size_bytes() > availableBytes) {
    // Return 0 to indicate failure
    return 0;
  }

  // Copy the data from 'buffer_' into 'buf' starting at the specified position
  // 'pos'
  std::copy(buffer_.begin() + pos, buffer_.begin() + pos + buf.size_bytes(),
            buf.begin());

  // Return the number of bytes read
  return buf.size_bytes();
}

int file::saveBuffer() {
  // Open file
  std::ofstream outputFile("output.bin", std::ios::out | std::ios::binary);
  if (!outputFile) {
    std::cerr << "Failed to open the file for writing" << std::endl;
    return 1;
  }
  // Write vector data to the file
  outputFile.write(reinterpret_cast<const char *>(buffer_.data()),
                   buffer_.size() * sizeof(std::byte));

  // Close the file
  outputFile.close();
  return 0;
}

void file::setState(bool state) { this->open_ = state; }

} // namespace redstone::disk
