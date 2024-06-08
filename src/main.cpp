#include <chrono>
#include <csignal>
#include <cstdio>
#include <fmt/format.h>
#include <map>
#include <memory>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <sys/random.h>
#include <sys/syscall.h>
#include <thread>
#include <toml++/toml.hpp>
#include <vector>

#include "hook/hook.hpp"
#include "metrics.hpp"
#include "sim/machine.hpp"
#include "sim/simulator.hpp"

uint64_t random_seed() {
  uint64_t buf;
  getrandom(&buf, sizeof(buf), 0);
  return buf;
}

struct config_replica {
  std::string path;
  std::vector<std::string> args;
  std::map<std::string, std::string> env;
  bool prime = false;
};

struct parsed_config {
  redstone::sim::options options;
  std::vector<config_replica> replicas;
};

std::vector<config_replica> parse_replicas(toml::array *arr) {
  if (!arr) {
    return {};
  }

  std::vector<config_replica> result;

  for (auto &elem : *arr) {
    auto table = elem.as_table();
    if (!table) {
      fprintf(stderr, "invalid configuration file\n");
      return {};
    }
    auto path = (*table)["path"].as_string();
    if (!path) {
      fprintf(stderr, "invalid configuration file\n");
    }

    std::vector<std::string> args{path->get()};

    config_replica replica = {
        .path = path->get(),
        .args = args,
        .env = {},
        .prime = (*table)["prime"].value_or(false),
    };

    result.push_back(replica);
  }
  return result;
}

parsed_config parse_config(const char *path) {
  auto config = toml::parse_file(path);
  auto min_latency_ns = config["net"]["min_latency_ns"].value_or(0);
  auto max_latency_ns = config["net"]["max_latency_ns"].value_or(0);
  auto replay_chance = config["net"]["replay_chance"].value_or(0.0);
  auto drop_chance = config["net"]["drop_chance"].value_or(0.0);

  auto time_scale = config["time"]["scale"].value_or(1.0);

  auto replicas = parse_replicas(config["replica"].as_array());

  return {
      .options =
          {
              .net_faults =
                  {
                      .min_latency = std::chrono::nanoseconds{min_latency_ns},
                      .max_latency = std::chrono::nanoseconds{max_latency_ns},
                      .p_drop = drop_chance,
                      .p_replay = replay_chance,
                  },
              .time_scale = config["time"]["scale"].value_or(1.0),
              .seed = config["seed"].value_or(random_seed()),
          },
      .replicas = replicas,
  };
}

int main(int argc, char **argv) {
  spdlog::set_level(spdlog::level::err);

  if (argc < 2) {
    spdlog::error("no simulation config path given");
    return 0;
  }
  spdlog::info("Hello, world!");

  auto config = parse_config(argv[1]);

  redstone::sim::simulator sim{std::move(config.options)};

  redstone::sim::runner_options options;

  redstone::sim::machine *prime = nullptr;
  std::vector<std::unique_ptr<redstone::sim::machine>> machines;

  for (auto &r : config.replicas) {
    options = {};
    options.path = r.path;
    options.args = r.args;
    auto m = std::make_unique<redstone::sim::machine>(sim, std::move(options));
    machines.push_back(std::move(m));

    if (r.prime) {
      prime = machines.back().get();
    }
  }

  for (auto &m : machines) {
    m->start();
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }

  if (prime == nullptr) {
    for (auto &m : machines) {
      m->current_replica()->runner().wait();
    }
  } else {
    prime->current_replica()->runner().wait();
    for (auto &m : machines) {
      if (m.get() != prime) {
        m->current_replica()->runner().kill();
      }
    }
  }
  // // for (int i = 1; i < argc; ++i) {
  // //   options.args.push_back(argv[i]);
  // // }
  // // options.path = options.args.at(0);
  // // redstone::sim::machine mach_echo{sim, std::move(options)};

  // options = {};
  // options.path = "./build/demo-echo";
  // options.args = {options.path};
  // redstone::sim::machine mach_echo{sim, std::move(options)};

  // options = {};
  // options.path = "./build/demo-squawk";
  // options.args = {options.path};
  // redstone::sim::machine mach_squawk{sim, std::move(options)};

  // mach_echo.start();

  // std::this_thread::sleep_for(std::chrono::milliseconds{100});

  // mach_squawk.start();

  // std::this_thread::sleep_for(std::chrono::milliseconds{500});
  // mach.current_replica()->runner().kill(SIGKILL);
  // // mach_squawk.current_replica()->runner().wait();
  // mach_echo.current_replica()->runner().wait();

  // mach_squawk.current_replica()->runner().kill();
  // auto state = mach_echo.current_replica()->runner().state();

  // fmt::println("exited with state {}", state.value().index());

  // {
  //   auto handle = redstone::sim::ptrace_run(options);
  //   handle->wait();
  // }

  // spdlog::info("done");

  // redstone::hook::print_stats();
  // redstone::metrics::dump();

  return 0;
}
