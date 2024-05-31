#include "hook_impls.hpp"

#include <sys/syscall.h>

#include <algorithm>
#include <iterator>
#include <span>
#include <utility>
#include <vector>

namespace redstone {
namespace {
hook_result explicit_passthrough(simulator &, instance &,
                                 std::span<const uint64_t, 6>) {
  return {.kind = hook_result_kind::passthrough};
}

static constexpr uint64_t passthroughs[] = {
    SYS_exit,
    SYS_exit_group,
    SYS_mmap,
    SYS_mprotect,
};

// Helper map to build the proper hook table
static constexpr std::pair<uint64_t, hook> hook_map[] = {
    {SYS_exit_group, explicit_passthrough},
    {SYS_mmap, explicit_passthrough},
};

static std::vector<hook> build_hook_table() {
  std::vector map{std::begin(hook_map), std::end(hook_map)};

  for (auto passthrough : passthroughs) {
    map.emplace_back(passthrough, explicit_passthrough);
  }

  auto max_sysno = std::max(std::begin(hook_map), std::end(hook_map),
                            [](const auto lhs, const auto rhs) {
                              return lhs->first < rhs->first;
                            })
                       ->first;

  std::vector<hook> table;
  table.resize(max_sysno + 1);

  for (auto &[sys, hook] : hook_map) {
    table[sys] = hook;
  }

  return table;
}
} // namespace

std::span<const hook> hook_table() {
  static const std::vector<hook> table = build_hook_table();
  return table;
}

hook get_hook(uint64_t sys) {
  auto table = hook_table();

  if (sys < table.size()) {
    return table[sys];
  }
  return nullptr;
}
} // namespace redstone