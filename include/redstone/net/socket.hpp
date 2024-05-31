#pragma once

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <queue>
#include <span>
#include <vector>

namespace redstone::net {
using packet = std::vector<std::byte>;

class connection_inner {
public:
private:
  std::queue<packet> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
};

class tcp_socket {
public:
  std::int64_t send(std::span<const std::byte> buf);
  std::int64_t recv(std::span<std::byte> buf);

private:
};
} // namespace redstone::net