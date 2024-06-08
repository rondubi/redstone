#include "impls.hpp"
#include "hook.hpp"
#include "metrics.hpp"
#include "net/datagram_socket.hpp"
#include "net/socket.hpp"
#include "net/stream_socket.hpp"
#include "sim/replica.hpp"

#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <memory>
#include <netinet/ip.h>
#include <span>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <vector>

namespace redstone::hook {
namespace {
auto store_fragment_callback(sim::replica &replica, uintptr_t addr) {
  return [&replica, addr](auto fragment) mutable {
    auto &runner = replica.runner();
    auto res = runner.read_memory(addr, fragment);
    if (0 <= res) {
      addr += res;
    }
    return res;
  };
}

auto load_fragment_callback(sim::replica &replica, uintptr_t addr) {
  return [&replica, addr](auto fragment) mutable {
    auto &runner = replica.runner();
    auto res = runner.write_memory(addr, fragment);
    if (0 <= res) {
      addr += res;
    }
    return res;
  };
}

net::socket_addr parse_addr(sim::replica &replica, uintptr_t dest_addr,
                            size_t dest_len, std::error_code &err) {
  err = {};

  struct sockaddr_storage addr_storage;

  if (sizeof(addr_storage) < dest_len || dest_len < 2) {
    err = std::error_code{EINVAL, std::generic_category()};
    return {};
  }

  std::vector<std::byte> dest{dest_len, std::byte{}};
  if (0 > replica.runner().read_memory(dest_addr, dest)) {
    err = std::error_code{EFAULT, std::generic_category()};
    return {};
  }

  memcpy(&addr_storage, dest.data(), dest.size());

  net::address_family af;

  switch (addr_storage.ss_family) {
  case AF_INET: {
    auto in_addr = reinterpret_cast<const sockaddr_in *>(&addr_storage);
    net::ipv4_addr ip_addr{};

    memcpy(ip_addr.octets, &in_addr->sin_addr, 4);
    ip_addr.port = in_addr->sin_port;

    return ip_addr;
  }
  case AF_UNIX: {
    auto un_addr = reinterpret_cast<const sockaddr_un *>(&addr_storage);

    std::string path;

    for (size_t i = offsetof(struct sockaddr_un, sun_path); i < dest_len; ++i) {
      char c = un_addr->sun_path[i];
      if (c == '\0')
        break;
      if (i == dest_len - 1) {
        err = std::error_code{EINVAL, std::generic_category()};
        return {};
      }
      path.push_back(c);
    }

    return net::unix_addr{std::move(path)};
  }
  default: {
    err = std::error_code{ENOTSUP, std::generic_category()};
    return {};
  }
  }
}
} // namespace

hook_result sys_write(sim::replica &replica,
                      std::span<const std::uint64_t, 6> args) {
  int fd = args[0];
  uintptr_t ptr = args[1];
  size_t len = args[2];

  auto &fd_table = replica.fd_table();

  std::shared_ptr<sim::file_descriptor> fildes;

  if (!fd_table.is_simulated(fd)) {
    return passthrough;
  }

  fildes = fd_table.get(fd);
  if (!fildes) {
    return error{EBADF};
  }

  auto store = store_fragment_callback(replica, ptr);
  auto res = fildes->write(store, len);
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
  auto _t = metrics::sys_close.start();

  int fd = args[0];

  if (!sim::file_descriptor_table::is_simulated(fd))
    return passthrough;

  auto &fd_table = replica.fd_table();

  return handled{fd_table.close(fd)};
}

hook_result sys_open(sim::replica &replica,
                     std::span<const std::uint64_t> args) {
  uintptr_t path = args[0];
  int oflag = args[1];
  int mode = args[2];

  return passthrough;
}

hook_result sys_socket(sim::replica &replica,
                       std::span<const std::uint64_t, 6> args) {
  auto _t = metrics::sys_socket.start();

  int domain = args[0];
  int type = args[1];
  int proto = args[2];

  net::address_family af;

  switch (domain) {
  case AF_INET:
    af = net::address_family::ipv4;
    break;
  case AF_UNIX:
    af = net::address_family::unix_;
    break;
  default:
    spdlog::warn("unsupported socket address family {}", domain);
    return error{EINVAL};
  }

  net::socket_type socket_type;

  std::shared_ptr<sim::file_descriptor> sock;

  switch (type) {
  case SOCK_STREAM:
    sock = std::make_shared<net::stream_socket>(af);
    break;
  case SOCK_DGRAM:
    sock = std::make_shared<net::datagram_socket>(af, replica.sim());
    break;
  default:
    spdlog::warn("unsupported socket type {}", type);
    return error{EINVAL};
  }

  auto fd = replica.fd_table().insert(sock);
  if (fd < 0) {
    return error{EMFILE};
  }

  return handled{fd};
}

hook_result sys_sendto(sim::replica &replica,
                       std::span<const std::uint64_t, 6> args) {
  const int socket_fd = args[0];
  const uintptr_t buffer = args[1];
  const size_t length = args[2];
  const int flags = args[3];
  const uintptr_t dest_addr = args[4];
  const size_t dest_len = args[5];

  if (!replica.fd_table().is_simulated(socket_fd))
    return passthrough;

  std::error_code err;
  const auto dst_addr = parse_addr(replica, dest_addr, dest_len, err);

  if (err) {
    return error{err.value()};
  }

  auto fd = replica.fd_table().get(socket_fd);
  if (!fd) {
    return error{EBADF};
  }

  auto socket = std::dynamic_pointer_cast<net::datagram_socket>(fd);
  if (!socket) {
    return error{ENOTSOCK};
  }

  auto read_data_callback = store_fragment_callback(replica, buffer);
  return handled{socket->send_to(read_data_callback, length, dst_addr)};
}

hook_result sys_recvfrom(sim::replica &replica,
                         std::span<const std::uint64_t, 6> args) {
  const int socket_fd = args[0];
  const uintptr_t buffer = args[1];
  const size_t length = args[2];
  const int flags = args[3];
  const uintptr_t dest_addr = args[4];
  const size_t dest_len = args[5];

  if (!replica.fd_table().is_simulated(socket_fd))
    return passthrough;

  // std::error_code err;
  // const auto dst_addr = parse_addr(replica, dest_addr, dest_len, err);

  // if (err) {
  //   return error{err.value()};
  // }

  auto fd = replica.fd_table().get(socket_fd);
  if (!fd) {
    return error{EBADF};
  }

  auto socket = std::dynamic_pointer_cast<net::datagram_socket>(fd);
  if (!socket) {
    return error{ENOTSOCK};
  }

  auto write_data_callback = load_fragment_callback(replica, buffer);
  net::socket_addr addr;
  auto res = socket->recv_from(write_data_callback, length, &addr);
  return handled{res};
}

hook_result sys_connect(sim::replica &replica,
                        std::span<const std::uint64_t, 6> args) {
  int sockfd = args[0];
  uintptr_t addr = args[1];
  size_t addr_len = args[2];

  if (!replica.fd_table().is_simulated(sockfd)) {
    return passthrough;
  }

  std::error_code err;
  auto sock_addr = parse_addr(replica, addr, addr_len, err);
  if (err) {
    return error{err.value()};
  }

  auto sock = replica.fd_table().get(sockfd);
  if (!sock) {
    return error{EBADF};
  }
  auto s = std::dynamic_pointer_cast<net::socket>(sock);
  if (!s) {
    return error{ENOTSOCK};
  }

  auto peer = replica.network().get(sock_addr);

  if (!peer) {
    return error{ECONNREFUSED};
  }

  auto s2 = std::dynamic_pointer_cast<net::stream_socket>(s);

  if (!s2) {
    return error{EOPNOTSUPP};
  }

  auto peer2 = std::dynamic_pointer_cast<net::stream_socket>(peer);
  if (!peer2 || peer2->listening()) {
    return error{ECONNREFUSED};
  }
  return handled{s2->connect(peer2)};
}

hook_result sys_bind(sim::replica &replica,
                     std::span<const std::uint64_t, 6> args) {
  int arg_socket_fd = args[0];
  uintptr_t arg_addr = args[1];
  size_t arg_addr_len = args[2];

  if (!replica.fd_table().is_simulated(arg_socket_fd)) {
    return passthrough;
  }

  auto fd = replica.fd_table().get(arg_socket_fd);
  if (!fd) {
    return error{EBADF};
  }

  auto sock = std::dynamic_pointer_cast<net::socket>(fd);
  if (!sock) {
    return error{ENOTSOCK};
  }

  std::error_code err;
  auto addr = parse_addr(replica, arg_addr, arg_addr_len, err);

  if (err) {
    return error{err.value()};
  }

  err = replica.network().bind(addr, sock);

  if (err) {
    return error{err.value()};
  }

  return handled{0};
}

hook_result sys_clock_gettime(sim::replica &replica,
                              std::span<const std::uint64_t, 6> args);

hook_result sys_clock_nanosleep(sim::replica &replica,
                                std::span<const std::uint64_t, 6> args) {
  const int arg_clockid = args[0];
  const int arg_flags = args[1];
  const uintptr_t arg_request = args[2];
  const uintptr_t arg_remain = args[3];

  timespec tm;

  auto res = replica.runner().read_memory(
      arg_request, std::as_writable_bytes(std::span<timespec, 1>{&tm, 1}));
  if (res < 0) {
    return error{-res};
  }
  auto time_scale = replica.sim().initial_options().time_scale;

  auto duration =
      std::chrono::seconds{tm.tv_sec} + std::chrono::nanoseconds{tm.tv_nsec};

  std::this_thread::sleep_for(duration * time_scale);
  return handled{0};
}

hook_result sys_clock_gettime(sim::replica &replica,
                              std::span<const std::uint64_t, 6> args) {
  const int arg_clockid = args[0];
  const uintptr_t arg_res = args[1];

  auto now = sim::clock::now();
  auto time_scale = replica.sim().initial_options().time_scale;
  auto since_epoch = sim::clock::now() - replica.epoch();
  auto scaled_since_epoch = since_epoch / time_scale;
  auto new_time = replica.epoch() + scaled_since_epoch;

  auto tv_sec = std::chrono::duration_cast<std::chrono::seconds>(
      new_time.time_since_epoch());
  auto tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(
      new_time.time_since_epoch() - tv_sec);

  timespec tm = {
      .tv_sec = tv_sec.count(),
      .tv_nsec = tv_nsec.count(),
  };

  auto res = replica.runner().write_memory(
      arg_res, std::as_bytes(std::span<timespec>{&tm, 1}));

  if (res < 0) {
    return error{-res};
  }
  return handled{0};
}
} // namespace redstone::hook