#include "redstone/net/udp.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

constexpr std::size_t max_udp_packet_size =
    std::numeric_limits<uint16_t>::max() - 128;

namespace redstone::net {
std::int64_t udp_stream::write(std::span<const std::byte> data) {
  if (max_udp_packet_size < data.size_bytes())
    data = data.subspan(0, max_udp_packet_size);

  if (fault_options_.should_drop(rng_))
    return data.size();

  const auto now = std::chrono::steady_clock::now();
  const auto latency = fault_options_.latency(rng_);
  const auto arrival = now + latency;

  udp_packet p{
      .arrival = arrival,
      .bytes = std::vector<std::byte>{data.begin(), data.end()},
  };

  std::vector<udp_packet> replays;
  const auto replay_count = fault_options_.replay_count(rng_);

  while (replays.size() < replay_count) {
    replays.push_back(p);
    replays.back().arrival = now + fault_options_.latency(rng_);
  }

  std::unique_lock lock{mutex_};

  packets_.push(std::move(p));
  for (auto &replay : replays) {
    packets_.push(std::move(replay));
  }

  cond_.notify_one();

  lock.unlock();

  for (auto &listener : select_write_)
    listener();

  return data.size_bytes();
}

std::int64_t udp_stream::read(std::span<std::byte> buf) {
  std::unique_lock lock{mutex_};

  while (!buf.empty()) {
    if (packets_.empty()) {
      cond_.wait(lock);
      continue;
    }

    auto &p = packets_.top();

    if (p.arrival > std::chrono::steady_clock::now()) {
      cond_.wait_until(lock, p.arrival);
      continue;
    }

    auto data = p.remaining();
    if (data.empty()) {
      packets_.pop();
      continue;
    }

    if (data.size_bytes() > buf.size_bytes())
      data = data.subspan(0, buf.size_bytes());

    std::copy(data.begin(), data.end(), buf.begin());
    p.consumed += data.size_bytes();

    if (p.remaining().empty()) {
      packets_.pop();
    }

    return data.size_bytes();
  }

  return 0;
}
} // namespace redstone::net
