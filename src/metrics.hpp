#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

namespace redstone::metrics {
struct metric;

struct timed {
  ~timed() { stop(); }

  void stop();

  metric *m;
  std::chrono::high_resolution_clock::time_point start;
};

struct metric {
  std::atomic<std::uint64_t> total_time = 0;
  std::atomic<std::uint64_t> count = 0;

  timed start() {
    auto now = std::chrono::high_resolution_clock::now();
    return timed{this, now};
  }

  void add_sample(std::chrono::nanoseconds sample) {
    total_time += sample.count();
    count += 1;
  }

  std::chrono::nanoseconds average() const {
    return std::chrono::nanoseconds{total_time / count};
  }
};

inline void timed::stop() {
  auto end = std::chrono::high_resolution_clock::now();
  auto sample = start - end;
  if (m != nullptr) {
    m->add_sample(sample);
  }
  m = nullptr;
}

static metric sys_socket;
static metric sys_close;

void dump();
} // namespace redstone::metrics