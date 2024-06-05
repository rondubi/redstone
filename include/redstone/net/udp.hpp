#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <span>
#include <vector>

#include "fault.hpp"
#include "redstone/random/xoshiro.hpp"

namespace redstone::net {
// A one-way UDP stream
class udp_stream {
public:
  std::int64_t write(std::span<const std::byte> packet);
  std::int64_t read(std::span<std::byte> buf);

private:
  struct udp_packet {
    std::chrono::steady_clock::time_point arrival;
    std::vector<std::byte> bytes;
    mutable std::size_t consumed = 0;

    friend bool operator<(const udp_packet &lhs, const udp_packet &rhs) {
      // Flip ordering
      return lhs.arrival > rhs.arrival;
    }

    friend bool operator>(const udp_packet &lhs, const udp_packet &rhs) {
      return rhs < lhs;
    }

    friend bool operator<=(const udp_packet &lhs, const udp_packet &rhs) {
      return !(rhs < lhs);
    }

    friend bool operator>=(const udp_packet &lhs, const udp_packet &rhs) {
      return !(lhs < rhs);
    }

    std::span<const std::byte> remaining() const {
      std::span<const std::byte> data = bytes;
      return data.subspan(consumed);
    }
  };

  std::mutex mutex_;
  std::condition_variable cond_;
  std::priority_queue<udp_packet> packets_;

  std::vector<std::function<void()>> select_read_;
  std::vector<std::function<void()>> select_write_;

  const net_fault_options fault_options_;
  // const std::chrono::nanoseconds min_latency_{1000};
  // const std::chrono::nanoseconds max_latency_{1001};
  // const double drop_chance_ = 0.01;

  // Only accessed by writer
  random::xoshiro256_star_star rng_;

  void send_packet(std::span<const std::byte> packet);
  udp_packet recv_packet();
};
} // namespace redstone::net