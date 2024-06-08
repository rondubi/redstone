#pragma once

#include "sim/file_descriptor.hpp"
#include "socket.hpp"

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <span>
#include <tl/function_ref.hpp>

namespace redstone::net {
class stream_socket : public socket {
public:
  explicit stream_socket(address_family af) : socket{af, socket_type::stream} {}

  explicit stream_socket(address_family af, std::shared_ptr<stream_socket> peer)
      : socket{af, socket_type::stream}, peer_{peer} {}

  std::int64_t
  recv(tl::function_ref<int(std::span<const std::byte>)> write_data_callback,
       std::size_t bytes);

  std::int64_t
  send(tl::function_ref<int(std::span<std::byte>)> read_data_callback,
       std::size_t bytes);

  int connect(std::shared_ptr<stream_socket> other);

  bool listening() const { return listening_; }

private:
  std::weak_ptr<stream_socket> peer_;
  std::queue<std::byte> buffer_;
  std::condition_variable cond_;
  std::mutex mutex_;
  bool listening_ = false;
};
} // namespace redstone::net