#include <sys/ptrace.h>
#include <system_error>

#include "child.hpp"
#include "ptrace.hpp"

namespace redstone::sys::ptrace {
namespace {
template <typename T, typename U>
void ptrace_child(child &child, enum __ptrace_request req, T addr, U data) {
  if (0 > ::ptrace(req, child.pid(), addr, data))
    throw std::system_error{errno, std::generic_category(), "ptrace failed"};
}
} // namespace

void syscall(child &c) { ptrace_child(c, PTRACE_SYSCALL, 0, 0); }

user_regs get_regs(child &c) {
  user_regs regs;
  ptrace_child(c, PTRACE_GETREGS, 0, regs);
  return regs;
}

void set_regs(child &c, const user_regs &regs) {
  ptrace_child(c, PTRACE_SETREGS, 0, &regs);
}

void set_options(child &c, int options) {
  ptrace_child(c, PTRACE_SETOPTIONS, 0, options);
}

void get_syscall_info(child &c, syscall_info *info) {
  ptrace_child(c, PTRACE_GET_SYSCALL_INFO, sizeof(*info), info);
}
} // namespace redstone::sys::ptrace