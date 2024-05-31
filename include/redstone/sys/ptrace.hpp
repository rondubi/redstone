#pragma once

#include <sys/user.h>

#include <cstdint>

namespace redstone::sys {
class child;

namespace ptrace {
struct syscall_info {
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

using user_regs = ::user_regs_struct;

void syscall(child &c);
user_regs get_regs(child &c);
void set_regs(child &c, const user_regs &regs);
void set_options(child &c, int options);
void get_syscall_info(child &c, syscall_info *info);
} // namespace ptrace
} // namespace redstone::sys