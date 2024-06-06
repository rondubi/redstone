#include "hook/hook.hpp"

#include <mutex>
#include <set>
#include <string_view>
#include <sys/auxv.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <span>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

#include "hook/syscalls.hpp"
#include "impls.hpp"

namespace redstone::hook {
namespace {
hook_result explicit_passthrough(sim::replica &, std::span<const uint64_t, 6>) {
  return passthrough;
}

static constexpr uint64_t passthroughs[] = {
    SYS_exit,
    SYS_exit_group,
    SYS_mmap,
    SYS_mprotect,
    SYS_munmap,
    SYS_rt_sigaction,
    SYS_set_tid_address,
    SYS_set_robust_list,
    SYS_rt_sigprocmask,
    SYS_prlimit64,
    SYS_brk,
    SYS_arch_prctl,
};

// Helper map to build the proper hook table
// static constexpr std::pair<uint64_t, hook> hook_map[] = {
//     {SYS_exit_group, explicit_passthrough},
//     {SYS_mmap, explicit_passthrough},
// };

static std::vector<hook> build_hook_table() {
  std::vector<std::pair<uint64_t, hook>> map{
      {SYS_write, sys_write},
      {SYS_read, sys_read},
      {SYS_close, sys_close},
  };

  for (auto passthrough : passthroughs) {
    std::pair<uint64_t, hook> pair{passthrough, explicit_passthrough};

    fmt::println("passthrough {}", syscall_name(passthrough));
    map.push_back(pair);
  }

  auto max_sysno =
      std::max(map.begin(), map.end(), [](const auto &lhs, const auto &rhs) {
        return lhs->first < rhs->first;
      })->first;

  std::vector<hook> table;
  table.resize(max_sysno + 1, nullptr);

  for (auto &[sys, hook] : map) {
    if (table.size() <= sys) {
      table.resize(sys + 1);
    }
    table.at(sys) = hook;
  }

  return table;
}

} // namespace

std::span<const hook> hook_table() {
  static const std::vector<hook> table = build_hook_table();
  return table;
}

namespace {
std::mutex mutex;
std::set<std::string_view> unimplemented;
std::set<std::string_view> all;
std::set<std::string_view> passthrough_set;
std::set<std::string_view> implemented;
} // namespace

hook get_hook(uint64_t sys) {
  std::unique_lock lock{mutex};

  auto name = syscall_name(sys);
  all.insert(name);

  auto table = hook_table();

  hook h = nullptr;

  if (sys < table.size()) {
    h = table[sys];
  }

  if (h == explicit_passthrough) {
    spdlog::trace("passthrough: {}", name);
    passthrough_set.insert(name);
  } else if (h == nullptr) {
    spdlog::warn("unimplemented: {}", name);
    unimplemented.insert(name);
  } else {
    spdlog::trace("implemented: {}", name);
    implemented.insert(name);
  }
  return h;
}

void print_stats() {
  std::unique_lock lock{mutex};

  fmt::println("passthrough:");
  for (auto sys : passthrough_set) {
    fmt::println("\t{}", sys);
  }

  fmt::println("implemented:");
  for (auto sys : implemented) {
    fmt::println("\t{}", sys);
  }

  fmt::println("unimplemented:");
  for (auto sys : unimplemented) {
    fmt::println("\t{}", sys);
  }
}

} // namespace redstone::hook