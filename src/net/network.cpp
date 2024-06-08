#include "network.hpp"
#include <cerrno>
#include <system_error>

namespace redstone::net {
std::error_code network::bind(socket_addr addr, std::shared_ptr<socket> sock) {
  std::unique_lock lock{mutex_};

  auto &slot = map_[addr];

  if (slot.lock() != nullptr) {
    return std::error_code{EADDRNOTAVAIL, std::generic_category()};
  }

  slot = sock;
  return {};
}

void network::clear(const socket_addr &addr, socket *s) {
  std::unique_lock lock{mutex_};

  auto it = map_.find(addr);

  if (it == map_.end()) {
    return;
  }

  auto p = it->second.lock();
  if (p != nullptr && p.get() != s)
    return;

  map_.erase(it);
}

std::shared_ptr<socket> network::get(const socket_addr &addr) {
  std::unique_lock lock{mutex_};
  return map_[addr].lock();
}
} // namespace redstone::net