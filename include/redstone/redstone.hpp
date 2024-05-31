#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "redstone/net/network.hpp"
#include "redstone/net/socket.hpp"
#include "redstone/random/xoshiro.hpp"
#include "redstone/sim/runner.hpp"
#include "redstone/sys/child.hpp"

namespace redstone {
struct child {
  sys::child process;
  std::thread thread;
};

struct instance_metrics {
  uint64_t crashes_forced = 0;
  uint64_t crashes_bugs = 0;
};

struct instance {
  // Should be initialized

  std::string path;
  std::vector<std::string> args;
  std::vector<std::string> env;
  random::xoshiro256_star_star rng;

  // Can be default initialized
  std::shared_ptr<child> child;
  instance_metrics metrics;

  ~instance() {
    if (child) {
      std::fputs("instance destroyed while child running", stderr);
      std::abort();
    }
  }
};

class simulator {
public:
  explicit simulator(uint64_t seed);

  instance &push_instance(std::string path);

  void run();

private:
  std::mutex sim_mutex_;

  // Root rng that all other seeds are generated from
  random::xoshiro256_star_star root_rng_;

  // A vector of instances, some running some not
  std::vector<instance> instances_;

  // Fuck you dave
  void dave_tripped_on_a_power_cable();
};
} // namespace redstone