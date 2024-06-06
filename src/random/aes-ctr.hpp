#pragma once

#include <cstdint>

#include <immintrin.h>

namespace redstone::random {
class aes_ctr {
public:
private:
  std::uint64_t ctr_;
};
} // namespace redstone::random