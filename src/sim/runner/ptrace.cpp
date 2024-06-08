#include "sim/runner.hpp"

#include <cassert>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <sys/auxv.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <variant>
#include <vector>

#include <spdlog/spdlog.h>

#include "hook/hook.hpp"
#include "hook/syscalls.hpp"
#include "sim/machine.hpp"
#include "sim/replica.hpp"
#include "sys/child.hpp"
#include "sys/file.hpp"
#include "sys/ptrace.hpp"

namespace redstone::sim {

class barrier {
public:
  explicit barrier(std::size_t count) : count_{count} {}

  void wait() {
    std::unique_lock lock{mutex_};
    assert(current_ != 0);

    current_--;

    if (current_ == 0) {
      current_ = count_;
      epoch_++;
      cond_.notify_all();
      return;
    }

    auto epoch = epoch_;

    while (epoch == epoch_) {
      cond_.wait(lock);
    }
  }

private:
  std::size_t count_;
  std::size_t current_ = count_;
  std::size_t epoch_ = 0;
  std::mutex mutex_;
  std::condition_variable cond_;
};
namespace {
sys::child spawn(const runner_options &options) {
  if (options.args.at(0) != options.path) {
    throw std::invalid_argument{
        "spawn: argv[0] must be the same as the provided path"};
  }

  std::vector<char *> argv;

  for (auto &arg : options.args) {
    argv.push_back(const_cast<char *>(arg.c_str()));
  }
  argv.push_back(nullptr);

  std::vector<char *> env;

  for (auto &var : options.env) {
    env.push_back(const_cast<char *>(var.c_str()));
  }

  pid_t pid = fork();

  if (pid < 0) {
    throw std::system_error{errno, std::generic_category(),
                            "failed to fork process"};
  }

  if (0 < pid) {
    sys::child c{pid, sys::child::running{}};
    c.wait();
    return c;
  }

  ::ptrace(PTRACE_TRACEME, pid, 0, 0);
  std::raise(SIGSTOP);

  execvp(options.path.c_str(), argv.data());
  throw std::system_error{errno, std::generic_category(), "execvp failed"};
}

void replace_syscall_with_result(sys::child &child, uint64_t result) {
  // Overwrite syscall with SYS_gettid (noop, essentially)
  auto regs = sys::ptrace::get_regs(child);
  regs.orig_rax = SYS_gettid;
  regs.rax = SYS_gettid;
  sys::ptrace::set_regs(child, regs);

  // Wait for gettid to finish
  sys::ptrace::syscall(child);
  child.wait();

  // Update return value of 'gettid' to be the result we want
  regs.orig_rax = result;
  regs.rax = result;

  sys::ptrace::set_regs(child, regs);
}

void handle_trapped(sys::child &child, replica &replica) {
  sys::ptrace::syscall_info info;

  sys::ptrace::get_syscall_info(child, &info);

  if (info.op != PTRACE_SYSCALL_INFO_ENTRY) {
    return;
  }

  auto hook = hook::get_hook(info.entry.nr);

  if (hook == nullptr) {
    return;
  }

  auto result = hook(replica, info.entry.args);

  switch (result.kind) {
  case redstone::hook::hook_result_kind::handled:
    spdlog::trace("simulated syscall {}, result {}",
                  hook::syscall_name(info.entry.nr), result.handled);
    replace_syscall_with_result(child, result.handled);
    return;
  case redstone::hook::hook_result_kind::passthrough:
    return;
  }
}

void remove_vdso(sys::child &child) {
  uintptr_t pos = child.ptrace(PTRACE_PEEKUSER, sizeof(uintptr_t) * RSP,
                               static_cast<void *>(nullptr));

  for (int i = 0; i < 2;) {
    auto v = child.ptrace(PTRACE_PEEKDATA, pos += 8, nullptr);
    if (v == AT_NULL) {
      i++;
    }
  }

  for (;;) {
    auto v = child.ptrace(PTRACE_PEEKDATA, pos += 8, nullptr);
    if (v == AT_NULL) {
      break;
    }
    if (v == AT_SYSINFO_EHDR) {
      child.ptrace(PTRACE_POKEDATA, pos, AT_IGNORE);
      break;
    }
  }
}

void wait_until_execve(sys::child &child) {
  spdlog::info("waiting until child execve");

  while (true) {
    sys::ptrace::syscall(child);

    auto status = child.wait();

    if (status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXEC << 8))) {
      return;
    }
  }
}

struct ptrace_runner_handle : public runner_handle {
  ptrace_runner_handle() : runner_handle{} {}

  void kill(int signal) final { child.terminate(signal); }

  void wait() final {
    wait_inner();
    spdlog::info("made it!");
  }

  void wait_inner() {
    worker.join();
    //   std::unique_lock lock{mutex};

    //   for (;;) {
    //     spdlog::info("hi!");
    //     if (!saved_state.has_value()) {
    //       cond.wait(lock);
    //       spdlog::warn("done waiting");
    //       continue;
    //     }

    //     spdlog::info("hi 1!");
    //     auto state = saved_state.value();

    //     spdlog::info("hi 2!");

    //     if (std::holds_alternative<sys::child::exited>(state) ||
    //         std::holds_alternative<sys::child::terminated>(state)) {
    //       spdlog::info("hi 3!");
    //       redstone::hook::print_stats();
    //       return;
    //     }

    //     cond.wait(lock);
    //   }
  }

  std::optional<sys::child::run_state> state() final {
    std::scoped_lock _lock{mutex};
    return saved_state;
  }

  int write_memory(uintptr_t ptr, std::span<const std::byte> data) final {
    unsigned long word;

    while (data.size() >= sizeof(long)) {
      memcpy(&word, data.data(), sizeof(word));
      ::ptrace(PTRACE_POKEDATA, child.pid(), ptr, word);
      data = data.subspan(sizeof(word));
      ptr += sizeof(long);
    }
    if (!data.empty()) {
      auto word = ::ptrace(PTRACE_PEEKDATA, child.pid(), ptr, 0);
      memcpy(&word, data.data(), data.size());
      ::ptrace(PTRACE_POKEDATA, child.pid(), ptr, word);
    }
    return 0;
  }

  int read_memory(uintptr_t ptr, std::span<std::byte> data) final {
    while (data.size() >= sizeof(long)) {
      auto word = ::ptrace(PTRACE_PEEKDATA, child.pid(), ptr, 0);
      memcpy(data.data(), &word, sizeof(word));
      data = data.subspan(sizeof(long));
      ptr += sizeof(long);
    }
    if (!data.empty()) {
      auto word = ::ptrace(PTRACE_PEEKDATA, child.pid(), ptr, 0);
      memcpy(data.data(), &word, data.size());
    }
    return 0;
    // assert(memfd);

    // assert(memfd.lseek(ptr) == ptr);

    // auto res = memfd.read_exact(data);
    // if (res < 0) {
    //   std::cerr << "failed to read process memory "
    //             << std::error_code{static_cast<int>(-res),
    //                                std::generic_category()}
    //             << std::endl;

    //   return res;
    // }
    // return 0;
  }

  std::mutex mutex;
  std::condition_variable cond;
  std::thread worker;
  std::optional<sys::child::run_state> saved_state;
  sys::child child;
  sys::file memfd;
};

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

struct worker_args {
  std::shared_ptr<ptrace_runner_handle> handle;
  const runner_options *options;
  barrier *barrier;
};

void ptrace_worker(worker_args args) {
  int status;

  auto &child = args.handle->child;
  child = spawn(*args.options);
  args.handle->memfd = sys::proc_mem(child, O_RDWR);
  assert(args.handle->memfd);

  machine *machine = args.options->machine;
  assert(machine);

  sys::ptrace::set_options(child, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEEXEC);

  args.barrier->wait();

  // https://stackoverflow.com/questions/8280014/disabling-vsyscalls-in-linux/52402306#52402306
  wait_until_execve(child);
  remove_vdso(child);

  auto replica = machine->current_replica();
  while (!replica) {
    printf("replica = null\n");
    replica = machine->current_replica();
  }

  for (;;) {

    sys::ptrace::syscall(child);
    child.wait();

    auto state = child.state().value();

    {
      std::unique_lock lock{args.handle->mutex};
      args.handle->saved_state = state;
    }

    auto should_return =
        std::visit(overloaded{
                       [](sys::child::running v) { return false; },
                       [&replica, &child](sys::child::stopped v) {
                         if (v.signal == (0x80 | SIGTRAP)) {
                           assert(replica);
                           handle_trapped(child, *replica);
                         }
                         return false;
                       },
                       [](sys::child::terminated v) { return true; },
                       [](sys::child::exited v) { return true; },
                   },
                   state);

    if (should_return) {
      return;
    }
  }
}
} // namespace

std::shared_ptr<sim::runner_handle> ptrace_run(const runner_options &options) {
  auto handle = std::make_shared<ptrace_runner_handle>();

  std::condition_variable cond;

  barrier b{2};
  worker_args args{
      .handle = handle,
      .options = &options,
      .barrier = &b,
  };

  handle->worker = std::thread{[&args] {
    try {
      ptrace_worker(std::move(args));
    } catch (const std::system_error &err) {
      if (err.code() == std::error_code{ESRCH, std::generic_category()}) {
        spdlog::warn("child crashed");
        return;
      }
      fmt::println("caught error: {}", err.code().value());
      throw;
    }
  }};

  b.wait();
  return handle;
}
} // namespace redstone::sim