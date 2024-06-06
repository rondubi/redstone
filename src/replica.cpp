#include "replica.hpp"

#include <sys/syscall.h>

#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <variant>
#include <vector>

#include "hook/hook.hpp"

namespace redstone::replica {
using file_descriptor = std::variant<disk::open_file>;

struct child {
  sys::child process;
  std::map<int, std::shared_ptr<file_descriptor>> fds;
};

replica::~replica() = default;

namespace {
constexpr unsigned explicit_passthroughs[] = {SYS_mmap};

std::optional<std::uint64_t> passthrough(std::span<const std::uint64_t, 6>) {
  return std::nullopt;
}

class hook_handlers : redstone::hook::handlers {
public:
  using hook =
      std::optional<std::uint64_t> (*)(std::span<const std::uint64_t, 6>);

  explicit hook_handlers(child *child, replica *replica)
      : child_{child}, replica_{replica} {

    for (auto sysno : explicit_passthroughs) {
      set_explicit_passthrough(sysno);
    }
  }

  std::optional<std::uint64_t>
  maybe_handle(std::uint64_t sysno, std::span<const uint64_t, 6> args) final {
    if (sysno < hooks_table_.size()) {
      auto hook = hooks_table_[sysno];

      if (hook != nullptr) {
        return hook(args);
      }
    }
    return std::nullopt;
  }

  void set_explicit_passthrough(unsigned sysno) {
    set_handler(sysno, passthrough);
  }

  void set_handler(unsigned sysno, hook handler) {
    assert(handler != nullptr);

    if (hooks_table_.size() <= sysno) {
      hooks_table_.resize(sysno);
    }
    hooks_table_.at(sysno) = handler;
  }

private:
  std::vector<hook> hooks_table_;
  child *child_;
  replica *replica_;
};
} // namespace
} // namespace redstone::replica