#include "impls.hpp"
#include "sim/replica.hpp"

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <spdlog/spdlog.h>
#include <unistd.h>

namespace redstone::hook {
namespace {
void fd_write() {}
} // namespace

hook_result sys_write(sim::replica &replica,
                      std::span<const std::uint64_t, 6> args) {
  int fd = args[0];
  uintptr_t ptr = args[1];
  size_t len = args[2];

  auto &fd_table = replica.fd_table();

  std::shared_ptr<sim::file_descriptor> fildes;

  if (fd == STDOUT_FILENO) {
    fildes = sim::stdout_fd();
  } else {
    if (!fd_table.is_simulated(fd)) {
      return passthrough;
    }

    fildes = fd_table.get(fd);
    if (!fildes) {
      return error{EBADF};
    }
  }

  spdlog::trace("a");

  auto store = [&replica, addr = ptr](auto fragment) {
    printf("a1\n");
    auto &runner = replica.runner();
    fprintf(stderr, "b1\n");
    // auto res = runner.read_memory(addr, fragment);
    // printf("c1\n");
    return 0;
  };

  spdlog::trace("c");
  auto res = fildes->write(store, len);
  spdlog::trace("b");

  return handled{res};
}

hook_result sys_read(sim::replica &replica,
                     std::span<const std::uint64_t, 6> args) {
  int fd = args[0];
  uintptr_t ptr = args[1];
  size_t len = args[2];

  auto &fd_table = replica.fd_table();

  if (!fd_table.is_simulated(fd)) {
    return passthrough;
  }

  auto fildes = fd_table.get(fd);
  if (!fildes) {
    return error{EBADF};
  }

  auto store = [&replica, addr = ptr](auto fragment) {
    auto &runner = replica.runner();
    return runner.write_memory(addr, fragment);
  };

  auto res = fildes->read(store, len);
  return handled{res};
}

hook_result sys_close(sim::replica &replica,
                      std::span<const std::uint64_t, 6> args) {
  int fd = args[0];

  if (!sim::file_descriptor_table::is_simulated(fd))
    return passthrough;

  auto &fd_table = replica.fd_table();

  return handled{fd_table.close(fd)};
}
} // namespace redstone::hook