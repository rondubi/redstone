#include "redstone/redstone.hpp"

#include <spdlog/spdlog.h>
#include <vector>

#include "redstone/hook/hook.hpp"
#include "redstone/random/splitmix.hpp"

namespace redstone {
simulator::simulator(uint64_t seed) {
  spdlog::trace("construct new simulator with seed {:#x}", seed);

  random::split_mix splitmix{seed};
  root_rng_.seed_from(splitmix);
}

instance &simulator::push_instance(std::string path) {
  std::scoped_lock _lock{sim_mutex_};

  instances_.push_back(instance{.path = std::move(path)});

  instance &i = instances_.back();
  i.args.push_back(i.path);
  i.rng.seed_from(root_rng_);

  return i;
}

namespace {
struct handlers : hook::handlers {
  std::optional<std::uint64_t>
  maybe_handle(std::uint64_t sysno, std::span<const uint64_t, 6> args) final {
    return std::nullopt;
  }
};

// Handles running a single instance
void run_instance(simulator &sim, instance &self) {
  hook::command command{
      .path = self.path,
      .args = self.args,
      .env = self.env,
  };
}
} // namespace

void simulator::run() {
  std::vector<std::thread> running;

  for (auto &i : instances_) {
    running.emplace_back([this, &i] { run_instance(*this, i); });
  }

  for (auto &thread : running) {
    thread.join();
  }
}
} // namespace redstone