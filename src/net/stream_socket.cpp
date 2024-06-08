#include "stream_socket.hpp"
#include "fault.hpp"
#include "random/xoshiro.hpp"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <vector>

namespace redstone::net {
std::int64_t stream_socket::recv(
    tl::function_ref<int(std::span<const std::byte>)> write_data_callback,
    std::size_t bytes) {
  std::unique_lock lock{mutex_};

  for (;;) {
    if (buffer_.empty()) {
      cond_.wait(lock);
      continue;
    }

    const auto n = std::min(bytes, buffer_.size());

    std::vector<std::byte> tmp;
    tmp.reserve(n);
    while (tmp.size() < n) {
      tmp.push_back(buffer_.front());
      buffer_.pop();
    }

    auto res = write_data_callback(tmp);
    if (res < 0) {
      return res;
    }
    return n;
  }
}

std::int64_t stream_socket::send(
    tl::function_ref<int(std::span<std::byte>)> read_data_callback,
    std::size_t bytes) {
  auto peer = peer_.lock();

  std::unique_lock lock{peer->mutex_};

  std::size_t sent = 0;

  std::vector<std::byte> buf{bytes, std::byte{}};

  auto res = read_data_callback(buf);
  if (res < 0)
    return res;

  for (auto byte : buf)
    peer->buffer_.push(byte);

  peer->cond_.notify_all();

  return res;
}

int stream_socket::connect(std::shared_ptr<stream_socket> other) {
  if (listening_) {
    return -EOPNOTSUPP;
  }
  if (get_af() != other->get_af()) {
    return -EAFNOSUPPORT;
  }
  peer_ = other;
  return 0;
}
} // namespace redstone::net
