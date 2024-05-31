#pragma once

#include <cstdint>
#include <sys/ptrace.h>
#include <sys/types.h>

#include <optional>
#include <system_error>
#include <variant>

namespace redstone::sys {
class child {
public:
  struct stopped {
    int signal;
  };

  struct terminated {
    int signal;
  };

  struct exited {
    int status;
  };

  struct running {};

  struct none {};

  using run_state = std::variant<running, stopped, terminated, exited>;

  child() = default;

  explicit child(::pid_t pid, std::optional<run_state> state)
      : pid_{pid}, state_{state} {}

  explicit operator bool() const { return 0 <= pid(); }

  ::pid_t pid() const { return pid_; }

  int wait();

  void terminate(int signal);

  [[nodiscard]] std::optional<run_state> state() const { return state_; }

  template <typename T, typename U>
  std::uint64_t ptrace(__ptrace_request req, T addr, U data) {
    auto rc = ::ptrace(req, pid_, addr, data);
    if (rc < 0) {
      throw std::system_error{errno, std::generic_category(), "ptrace failed"};
    }
    return static_cast<std::uint64_t>(rc);
  }

private:
  ::pid_t pid_ = -1;
  std::optional<run_state> state_;
};
} // namespace redstone::sys