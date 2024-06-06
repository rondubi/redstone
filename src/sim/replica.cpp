#include "replica.hpp"
#include <memory>

namespace redstone::sim {
replica::~replica() = default;

replica::replica(std::shared_ptr<runner_handle> handle, machine *m)
    : handle_{std::move(handle)}, machine_{m} {}
} // namespace redstone::sim