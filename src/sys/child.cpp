#include "redstone/sys/child.hpp"

#include <cstdio>
#include <sys/wait.h>

#include <stdexcept>
#include <system_error>

namespace redstone::sys {
int child::wait() {
  state_ = std::nullopt;

  int status;
  int rc = waitpid(pid(), &status, 0);

  if (rc < 0) {
    throw std::system_error{errno, std::generic_category(), "waitpid failed"};
  }

  if (WIFCONTINUED(status)) {
    state_ = running{};
  } else if (WIFSTOPPED(status)) {
    state_ = stopped{.signal = WSTOPSIG(status)};
  } else if (WIFEXITED(status)) {
    state_ = exited{WEXITSTATUS(status)};
    pid_ = -1;
  } else if (WIFSIGNALED(status)) {
    state_ = terminated{WTERMSIG(status)};
    pid_ = -1;
  } else {
    throw std::runtime_error{"unknown waitpid status"};
  }

  return status;
}

void child::terminate(int signal) {
  if (!*this) {
    throw std::invalid_argument{"terminate of not running process"};
  }

  int rc = ::kill(pid(), signal);
  if (rc < 0) {
    throw std::system_error{errno, std::generic_category(), "kill failed"};
  }
}
} // namespace redstone::sys