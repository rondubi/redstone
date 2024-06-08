#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <queue>
#include <span>
#include <vector>

#include "fault.hpp"
#include "network.hpp"
#include "random/xoshiro.hpp"
#include "sim/simulator.hpp"
#include "socket.hpp"

namespace redstone::net {
// A one-way UDP stream
class datagram_pipe {
public:
  explicit datagram_pipe(const net_fault_options &fault_options)
      : fault_options_{fault_options} {}

  void send(std::vector<std::byte> packet, socket_addr from);
  std::pair<std::vector<std::byte>, socket_addr> recv();

  template <typename Rng>
  void seed(Rng &rng) {
    rng_.seed_from(rng);
  }

private:
  struct packet {
    std::chrono::steady_clock::time_point arrival;
    std::vector<std::byte> bytes;
    mutable std::size_t consumed = 0;
    socket_addr from;

    friend bool operator<(const packet &lhs, const packet &rhs) {
      // Flip ordering
      return lhs.arrival > rhs.arrival;
    }

    friend bool operator>(const packet &lhs, const packet &rhs) {
      return rhs < lhs;
    }

    friend bool operator<=(const packet &lhs, const packet &rhs) {
      return !(rhs < lhs);
    }

    friend bool operator>=(const packet &lhs, const packet &rhs) {
      return !(lhs < rhs);
    }

    std::span<const std::byte> remaining() const {
      std::span<const std::byte> data = bytes;
      return data.subspan(consumed);
    }
  };

  std::mutex mutex_;
  std::condition_variable cond_;
  std::priority_queue<packet> packets_;

  const net_fault_options fault_options_{};

  // Only accessed by writer
  random::xoshiro256_star_star rng_;

  void send_packet(std::span<const std::byte> packet);
  packet recv_packet();
};

class datagram_socket : public socket {
public:
  explicit datagram_socket(address_family af, sim::simulator &sim);

  std::int64_t
  send_to(tl::function_ref<int(std::span<std::byte>)> read_data_callback,
          std::size_t bytes, const socket_addr &dst);

  std::int64_t recv_from(
      tl::function_ref<int(std::span<const std::byte>)> write_data_callback,
      std::size_t bytes, socket_addr *dst);

  void deliver(std::vector<std::byte> dgram, socket_addr addr);

private:
  datagram_pipe inbound_;
  std::mutex mutex_;
  network *net_;
};
} // namespace redstone::net