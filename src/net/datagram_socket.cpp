#include "datagram_socket.hpp"
#include <cerrno>
#include <cstdio>
#include <fmt/format.h>
#include <memory>
#include <random>
#include <vector>

namespace redstone::net {
void datagram_pipe::send(std::vector<std::byte> data, socket_addr from) {
  assert(data.size() < max_udp_packet_size);

  if (fault_options_.should_drop(rng_))
    return;

  const auto now = std::chrono::steady_clock::now();
  const auto latency = fault_options_.latency(rng_);
  const auto arrival = now + latency;

  packet p{
      .arrival = arrival,
      .bytes = std::move(data),
      .from = std::move(from),
  };

  std::vector<packet> replays;
  const auto replay_count = fault_options_.replay_count(rng_);
  replays.reserve(replay_count);

  while (replays.size() < replay_count) {
    replays.push_back(p);
    replays.back().arrival = now + fault_options_.latency(rng_) * 2;
  }

  std::unique_lock lock{mutex_};

  packets_.push(std::move(p));
  for (auto &replay : replays) {
    packets_.push(std::move(replay));
  }

  cond_.notify_one();
  lock.unlock();
}

std::pair<std::vector<std::byte>, socket_addr> datagram_pipe::recv() {
  std::unique_lock lock{mutex_};

  while (true) {
    if (packets_.empty()) {
      cond_.wait(lock);
      continue;
    }

    auto &p = packets_.top();

    if (p.arrival > std::chrono::steady_clock::now()) {
      cond_.wait_until(lock, p.arrival);
      continue;
    }

    auto data = std::move(p.bytes);
    auto addr = std::move(p.from);
    packets_.pop();
    return {data, addr};
  }
}

void datagram_socket::deliver(std::vector<std::byte> dgram, socket_addr addr) {
  std::unique_lock lock{mutex_};
  inbound_.send(std::move(dgram), std::move(addr));
}

std::int64_t datagram_socket::send_to(
    tl::function_ref<int(std::span<std::byte>)> read_data_callback,
    std::size_t bytes, const socket_addr &dst) {
  auto sock = net_->get(dst);
  if (!sock) {
    return -EHOSTUNREACH;
  }

  auto dgram_sock = std::dynamic_pointer_cast<datagram_socket>(sock);
  if (!dgram_sock) {
    return -ECONNREFUSED;
  }

  std::vector<std::byte> buf{bytes, std::byte{}};
  auto res = read_data_callback(buf);
  if (res < 0) {
    return res;
  }

  dgram_sock->deliver(buf, {});
  return bytes;
}

std::int64_t datagram_socket::recv_from(
    tl::function_ref<int(std::span<const std::byte>)> write_data_callback,
    std::size_t bytes, socket_addr *dst) {
  auto [data, addr] = inbound_.recv();

  if (dst) {
    *dst = addr;
  }

  if (bytes < data.size()) {
    data.resize(bytes);
  }

  write_data_callback(data);
  return data.size();
}

datagram_socket::datagram_socket(address_family af, sim::simulator &sim)
    : socket{af, socket_type::stream}, net_{&sim.net()},
      inbound_{sim.initial_options().net_faults} {
  inbound_.seed(sim.rng());
}
} // namespace redstone::net