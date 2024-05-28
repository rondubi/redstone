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

// #include <cassert>
// #include <csignal>
// #include <cstdint>
// #include <cstdio>
// #include <cstdlib>
// #include <map>
// #include <optional>
// #include <set>
// #include <span>
// #include <sys/ptrace.h>
// #include <sys/syscall.h>
// #include <sys/user.h>
// #include <sys/wait.h>
// #include <system_error>
// #include <unistd.h>
// #include <vector>

// #include "redstone/net/net.hpp"
// #include "redstone/time/time.hpp"

// void check_result(long result) {
//   if (result < 0) {
//     throw std::system_error{errno, std::generic_category()};
//   }
// }

// struct ptrace_syscall_info {
//   uint8_t op;                   /* Type of system call stop */
//   uint32_t arch;                /* AUDIT_ARCH_* value; see seccomp(2) */
//   uint64_t instruction_pointer; /* CPU instruction pointer */
//   uint64_t stack_pointer;       /* CPU stack pointer */
//   union {
//     struct {            /* op == PTRACE_SYSCALL_INFO_ENTRY */
//       uint64_t nr;      /* System call number */
//       uint64_t args[6]; /* System call arguments */
//     } entry;
//     struct {            /* op == PTRACE_SYSCALL_INFO_EXIT */
//       int64_t rval;     /* System call return value */
//       uint8_t is_error; /* System call error flag;
//                         Boolean: does rval contain
//                         an error value (-ERRCODE) or
//                         a nonerror return value? */
//     } exit;
//     struct {             /* op == PTRACE_SYSCALL_INFO_SECCOMP */
//       uint64_t nr;       /* System call number */
//       uint64_t args[6];  /* System call arguments */
//       uint32_t ret_data; /* SECCOMP_RET_DATA portion
//                          of SECCOMP_RET_TRACE
//                          return value */
//     } seccomp;
//   };
// };

// using handler = int (*)(uint64_t[6]);

// struct syscall_definitions {
//   std::set<int> transparent;
//   std::map<int, handler> overrides;
// };

// std::map<int, handler> build_overrides() { return {}; }

// const std::map<int, handler> &overrides() {
//   static const auto map = build_overrides();
//   return map;
// }

// handler get_override(int sysno) {
//   auto map = overrides();
//   return map[sysno];
// }

// enum class syscall_kind {
//   handle,
//   transparent,
//   unknown,
// };

// struct syscall_hooks {
//   std::optional<int> handle(int sysno, std::span<const uint64_t, 6> args) {
//     printf("syscall: %d\n", sysno);

//     if (sysno == SYS_write && args[0] == 1) {
//       uintptr_t ptr = args[1];
//       size_t len = args[2];
//       printf("write stdout ptr = %p, len = %zu\n", (void *)ptr, len);
//       return len;
//     }

//     return std::nullopt;
//   }
// };

// void child() {
//   fprintf(stderr, "enter!\n");
//   check_result(ptrace(PTRACE_TRACEME));

//   kill(getpid(), SIGSTOP);

//   close(4);
//   fprintf(stderr, "hello!\n");
//   fflush(stderr);

//   char *argv[] = {"../test", nullptr};
//   check_result(execv(argv[0], argv));
// }

// void parent(pid_t pid) {
//   std::set<int> unhandled;

//   syscall_hooks hooks;

//   waitpid(pid, NULL, 0);
//   check_result(ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD));

//   for (;;) {
//     check_result(ptrace(PTRACE_SYSCALL, pid, 0, 0));
//     int status;
//     waitpid(pid, &status, 0);

//     if (WIFSTOPPED(status)) {
//       switch (WSTOPSIG(status) & 0x7f) {
//       case SIGSEGV:
//         abort();
//         break;
//       case SIGTRAP:
//         break;
//       default:
//         // printf("unknown signal %d\n", WSTOPSIG(status) & 0x7f);
//         break;
//       }
//     } else if (WIFEXITED(status)) {
//       return;
//     }

//     // printf("signal %d\n", WSTOPSIG(status));
//     // printf("signaled := %d\n", WIFSIGNALED(status));
//     // printf("exited := %d\n", WIFEXITED(status));
//     // printf("stopped := %d\n", WIFSTOPPED(status));

//     // if (WIFSTOPPED(status) && (WSTOPSIG(status) & 0x80) != 0)
//     //   break;

//     struct ptrace_syscall_info info;

//     check_result(ptrace(PTRACE_GET_SYSCALL_INFO, pid, sizeof(info), &info));

//     if (info.op != PTRACE_SYSCALL_INFO_ENTRY) {
//       continue;
//     }

//     // printf("SyscallInfo {\n\top: %d,\n\tnumber: %lu\n}\n", info.op,
//     //        info.entry.nr);

//     auto result = hooks.handle(info.entry.nr, info.entry.args);

//     if (!result.has_value()) {
//       continue;
//     }

//     user_regs_struct regs;

//     check_result(ptrace(PTRACE_GETREGS, pid, 0, &regs));
//     regs.rax = SYS_gettid;
//     regs.orig_rax = SYS_gettid;
//     check_result(ptrace(PTRACE_SETREGS, pid, 0, &regs));

//     check_result(ptrace(PTRACE_SYSCALL, pid, 0, 0));
//     waitpid(pid, &status, 0);

//     // check_result(ptrace(PTRACE_GETREGS, pid, 0, &regs));
//     regs.rax = result.value();
//     check_result(ptrace(PTRACE_SETREGS, pid, 0, &regs));
//   }
// }

// int main() {
//   // char *argv[] = {"../test", nullptr};
//   // check_result(execv(argv[0], argv));

//   pid_t pid = fork();
//   if (pid < 0) {
//     throw std::system_error{errno, std::generic_category()};
//   } else if (pid == 0) {
//     child();
//   } else {
//     parent(pid);
//   }
// }
