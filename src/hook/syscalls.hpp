#pragma once

#include <string_view>

namespace redstone::hook {
std::string_view syscall_name(int num);
} // namespace redstone::hook