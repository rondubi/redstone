#include "redstone/hook/hook.hpp"
#include "redstone/hook/syscalls.hpp"

#include <bitset>
#include <cstdio>
#include <fmt/format.h>

#include <sys/syscall.h>

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

  std::optional<int> maybe_handle(int sysno,
                                  std::span<const uint64_t, 6> args) override {
    if (should_passthrough(sysno)) {
      fmt::println("passthrough: {}", sysno);
      return std::nullopt;
    }

    if (sysno == SYS_write && args[0] == 1) {
      uintptr_t ptr = args[1];
      size_t len = args[2];
      fmt::println("write stdout ptr = {}, len = {}", (void *)ptr, len);
      return len;
    }

    auto name = redstone::hook::syscall_name(sysno);
    fmt::println("unhandled: {} ({})", sysno, name);
    return std::nullopt;
  }

  void exited(int status) override { fmt::println("exited: {}", status); }

private:
  bool should_passthrough(int sysno) const {
    if (sysno < 0 || passthrough.size() <= sysno) {
      return true;
    }
    return passthrough.test(sysno);
  }
};

int main(int argc, char **argv) {
  redstone::hook::command cmd;

  for (int i = 1; i < argc; ++i) {
    cmd.args.push_back(argv[i]);
  }
  cmd.path = cmd.args.at(0);

  logging_handler handler;
  redstone::hook::run_command(cmd, handler);
}
