#pragma once

#include "socket.hpp"
#include <memory>
#include <mutex>
#include <unordered_map>

namespace redstone::net {
class network {
public:
  network() = default;

  std::error_code bind(socket_addr addr, std::shared_ptr<socket>);

  void clear(const socket_addr &addr, socket *s);

  std::shared_ptr<socket> get(const socket_addr &addr);

private:
  std::mutex mutex_;
  std::unordered_map<socket_addr, std::weak_ptr<socket>> map_;
};
} // namespace redstone::net