#include "replica.hpp"
#include "machine.hpp"
#include "simulator.hpp"
#include <memory>

namespace redstone::sim {
replica::~replica() = default;

replica::replica(std::shared_ptr<runner_handle> handle, machine &m)
    : handle_{std::move(handle)}, machine_{&m} {}

net::network &replica::network() { return machine_->sim().net(); }

simulator &replica::sim() { return machine_->sim(); }
} // namespace redstone::sim