#pragma once

#include "sim/file_descriptor.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <span>
#include <string>
#include <variant>

namespace redstone::net {
enum address_family {
  ipv4,
  unix_,
};

struct ipv4_addr {
  uint16_t port;
  char octets[4];

  friend bool operator<(const ipv4_addr &lhs, const ipv4_addr &rhs) {
    if (memcmp(lhs.octets, rhs.octets, 4) < 0)
      return true;
    return lhs.port < rhs.port;
  }

  friend bool operator>(const ipv4_addr &lhs, const ipv4_addr &rhs) {
    return rhs < lhs;
  }
  friend bool operator<=(const ipv4_addr &lhs, const ipv4_addr &rhs) {
    return !(rhs < lhs);
  }
  friend bool operator>=(const ipv4_addr &lhs, const ipv4_addr &rhs) {
    return !(lhs < rhs);
  }

  friend bool operator==(const ipv4_addr &lhs, const ipv4_addr &rhs) {
    return lhs.port == rhs.port && memcmp(lhs.octets, rhs.octets, 4) == 0;
  }
  friend bool operator!=(const ipv4_addr &lhs, const ipv4_addr &rhs) {
    return !(lhs == rhs);
  }
};

struct unix_addr {
  std::string path;

  friend bool operator<(const unix_addr &lhs, const unix_addr &rhs) {
    return lhs.path < rhs.path;
  }
  friend bool operator>(const unix_addr &lhs, const unix_addr &rhs) {
    return rhs < lhs;
  }
  friend bool operator<=(const unix_addr &lhs, const unix_addr &rhs) {
    return !(rhs < lhs);
  }
  friend bool operator>=(const unix_addr &lhs, const unix_addr &rhs) {
    return !(lhs < rhs);
  }

  friend bool operator==(const unix_addr &lhs, const unix_addr &rhs) {
    return lhs.path == rhs.path;
  }
  friend bool operator!=(const unix_addr &lhs, const unix_addr &rhs) {
    return !(lhs == rhs);
  }
};

using socket_addr = std::variant<ipv4_addr, unix_addr>;

enum class socket_type {
  stream,
  datagram,
};

class socket : public sim::file_descriptor {
public:
  explicit socket(address_family af, socket_type ty) : af_{af}, type_{ty} {}

  address_family get_af() const { return af_; }

private:
  address_family af_;
  socket_type type_;
};
} // namespace redstone::net

template <>
struct std::hash<redstone::net::unix_addr> {
  uint64_t operator()(const redstone::net::unix_addr &addr) const {
    return std::hash<std::string>{}(addr.path);
  }
};

template <>
struct std::hash<redstone::net::ipv4_addr> {
  uint64_t operator()(const redstone::net::ipv4_addr &addr) const {
    const auto h1 = std::hash<char>{}(addr.octets[0]);
    const auto h2 = std::hash<char>{}(addr.octets[1]);
    const auto h3 = std::hash<char>{}(addr.octets[2]);
    const auto h4 = std::hash<char>{}(addr.octets[3]);
    const auto h5 = std::hash<uint16_t>{}(addr.port);

    return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
  }
};