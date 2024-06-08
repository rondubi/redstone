#include "machine.hpp"

namespace redstone::sim {
void machine::start() {
  runner_options_.machine = this;
  auto handle = runner_(runner_options_);
  current_ = std::make_shared<replica>(handle, *this);
}
} // namespace redstone::sim
