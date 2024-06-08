#pragma once

#include <cstdint>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <system_error>

static constexpr const char *path = "echo-file";
static constexpr uint64_t iters = 512;

inline sockaddr_un get_addr() {
  sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  // addr.sun_len = 3 + strlen(path);
  strncpy(addr.sun_path, path, sizeof(addr.sun_path));
  return addr;
}

inline void check_return_value(int64_t v) {
  if (v < 0) {
    throw std::system_error{errno, std::generic_category()};
  }
}