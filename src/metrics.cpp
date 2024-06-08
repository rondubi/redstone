#include "metrics.hpp"

#include <chrono>
#include <fmt/chrono.h>
#include <fmt/format.h>

namespace redstone::metrics {
void dump() {
  fmt::println("metrics:");
  fmt::println("sys_socket: average {}", sys_socket.average());
  fmt::println("sys_close: average {}", sys_close.average());
}

} // namespace redstone::metrics