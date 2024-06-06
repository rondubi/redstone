

#include <sys/syscall.h>

#include <bitset>
#include <cstdio>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "hook/hook.hpp"
#include "hook/syscalls.hpp"
#include "random/splitmix.hpp"
#include "redstone.hpp"

namespace {
constexpr int passthroughs[] = {
    14,  // rt_sigprocmask
    59,  // execve
    12,  // brk
    158, // arch_prctl
    9,   // mmap
    10,  // mprotect
    11,  // munmap
    39,  // getpid
    231, // exit_group
};
std::bitset<500> get_passthroughs() {
  std::bitset<500> all;

  for (int sysno : passthroughs) {
    all.set(sysno);
  }

  return all;
}
} // namespace

struct logging_handler : redstone::hook::handlers {
  std::bitset<500> passthrough = get_passthroughs();

  std::optional<std::uint64_t>
  maybe_handle(std::uint64_t sysno,
               std::span<const uint64_t, 6> args) override {
    if (should_passthrough(sysno)) {
      fmt::println("passthrough: {} ({})", sysno,
                   redstone::hook::syscall_name(sysno));
      return std::nullopt;
    }

    if (sysno == SYS_write && args[0] == 1) {
      uintptr_t ptr = args[1];
      size_t len = args[2];
      fmt::println("write stdout ptr = {}, len = {}", (void *)ptr, len);
      return len;
    }

    auto name = redstone::hook::syscall_name(sysno);
    spdlog::warn("unhandled: {} ({})", sysno, name);
    return std::nullopt;
  }

  void exited(int status) override { fmt::println("exited: {}", status); }

private:
  bool should_passthrough(std::uint64_t sysno) const {
    if (passthrough.size() <= sysno) {
      return true;
    }
    return passthrough.test(sysno);
  }
};

namespace {
void add_single_instance_from_args(redstone::simulator &sim, int argc,
                                   char **argv) {
  auto &instance = sim.push_instance(argv[1]);

  for (int i = 2; i < argc; ++i) {
    instance.args.push_back(argv[i]);
  }
}
} // namespace

int main(int argc, char **argv) {
  spdlog::set_level(spdlog::level::trace);

  spdlog::info("Hello, world!");

  uint64_t seed = 0xfeedbeef;
  redstone::random::split_mix split{seed};

  redstone::simulator simulator{0xfeedbeef};

  for (int i = 0; i < 5; ++i) {
    add_single_instance_from_args(simulator, argc, argv);
  }

  redstone::sim::runner_options options;

  for (int i = 1; i < argc; ++i) {
    options.args.push_back(argv[i]);
  }
  options.path = options.args.at(0);

  {
    auto handle = redstone::sim::ptrace_run(options);
    handle->wait();
  }

  spdlog::info("done");

  redstone::hook::print_stats();

  return 0;
}
