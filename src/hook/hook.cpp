#include "redstone/hook/hook.hpp"
#include <csignal>
#include <cstdlib>
#include <stdexcept>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>
#include <vector>

namespace redstone::hook {
namespace {
struct ptrace_syscall_info {
  uint8_t op;                   /* Type of system call stop */
  uint32_t arch;                /* AUDIT_ARCH_* value; see seccomp(2) */
  uint64_t instruction_pointer; /* CPU instruction pointer */
  uint64_t stack_pointer;       /* CPU stack pointer */
  union {
    struct {            /* op == PTRACE_SYSCALL_INFO_ENTRY */
      uint64_t nr;      /* System call number */
      uint64_t args[6]; /* System call arguments */
    } entry;
    struct {            /* op == PTRACE_SYSCALL_INFO_EXIT */
      int64_t rval;     /* System call return value */
      uint8_t is_error; /* System call error flag;
                        Boolean: does rval contain
                        an error value (-ERRCODE) or
                        a nonerror return value? */
    } exit;
    struct {             /* op == PTRACE_SYSCALL_INFO_SECCOMP */
      uint64_t nr;       /* System call number */
      uint64_t args[6];  /* System call arguments */
      uint32_t ret_data; /* SECCOMP_RET_DATA portion
                         of SECCOMP_RET_TRACE
                         return value */
    } seccomp;
  };
};

struct child {
  pid_t pid;

  void wait(int *status, int options = 0) {
    if (0 > waitpid(pid, status, options))
      throw std::system_error{errno, std::generic_category(), "waitpid failed"};
  }

  template <typename T, typename U>
  void ptrace(enum __ptrace_request req, T addr, U data) {
    if (0 > ::ptrace(req, pid, addr, data))
      throw std::system_error{errno, std::generic_category(), "ptrace failed"};
  }

  void ptrace_syscall() { ptrace(PTRACE_SYSCALL, 0, 0); }

  void ptrace_get_regs(user_regs_struct *regs) {
    ptrace(PTRACE_GETREGS, 0, regs);
  }

  void ptrace_set_regs(const user_regs_struct &regs) {
    ptrace(PTRACE_SETREGS, 0, &regs);
  }

  void ptrace_set_options(int options) {
    ptrace(PTRACE_SETOPTIONS, 0, options);
  }

  void ptrace_get_syscall_info(ptrace_syscall_info *info) {
    ptrace(PTRACE_GET_SYSCALL_INFO, sizeof(*info), info);
  }
};

child spawn(const command &cmd) {
  if (cmd.args.at(0) != cmd.path) {
    throw std::invalid_argument{
        "spawn: argv[0] must be the same as the provided path"};
  }

  std::vector<char *> argv;

  for (auto &arg : cmd.args) {
    argv.push_back(const_cast<char *>(arg.c_str()));
  }
  argv.push_back(nullptr);

  pid_t pid = fork();
  child c{pid};

  if (pid < 0) {
    throw std::system_error{errno, std::generic_category(),
                            "failed to fork process"};
  }

  if (0 < pid) {
    return c;
  }

  c.ptrace(PTRACE_TRACEME, 0, 0);
  std::raise(SIGSTOP);

  execvp(cmd.path.c_str(), argv.data());
  throw std::system_error{errno, std::generic_category(), "execvp failed"};
}

} // namespace

void run_command(const command &cmd, handlers &handlers) {
  int status;
  user_regs_struct regs;
  ptrace_syscall_info info;

  auto child = spawn(cmd);

  child.wait(nullptr);
  child.ptrace_set_options(PTRACE_O_TRACESYSGOOD);

  for (;;) {
    child.ptrace_syscall();
    child.wait(&status);

    if (WIFSIGNALED(status)) {
      handlers.signaled(WTERMSIG(status));
      return;
    }
    if (WIFEXITED(status)) {
      handlers.exited(WEXITSTATUS(status));
      return;
    }
    if (WIFSTOPPED(status)) {
      int sig = WSTOPSIG(status);

      if (sig != (0x80 | SIGTRAP)) {
        handlers.stopped(sig);
      }
    }

    child.ptrace_get_syscall_info(&info);

    if (info.op != PTRACE_SYSCALL_INFO_ENTRY) {
      continue;
    }

    auto result = handlers.maybe_handle(info.entry.nr, info.entry.args);

    if (!result.has_value()) {
      continue;
    }

    // Overwrite syscall with SYS_gettid (noop, essentially)
    child.ptrace_get_regs(&regs);
    regs.orig_rax = SYS_gettid;
    regs.rax = SYS_gettid;
    child.ptrace_set_regs(regs);

    // Wait for gettid to finish
    child.ptrace_syscall();
    child.wait(NULL);

    // Update return value of 'gettid' to be the result we want
    regs.orig_rax = *result;
    regs.rax = *result;
    child.ptrace_set_regs(regs);
  }
}
} // namespace redstone::hook