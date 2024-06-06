#pragma once

#include <cstdint>
#include <cstring>
#include <span>

namespace redstone::random {
class split_mix {
public:
  explicit split_mix(uint64_t seed, uint64_t gamma = 0x9e3779b97f4a7c15L)
      : seed_{seed}, gamma_{gamma} {}

  uint64_t next() { return mix64(next_seed()); }

  void fill_bytes(std::span<std::byte> buf) {
    while (8 <= buf.size_bytes()) {
      uint64_t block = next();
      std::memcpy(buf.data(), &block, sizeof(block));
      buf = buf.subspan(8);
    }

    uint64_t block = next();
    std::memcpy(buf.data(), &block, buf.size_bytes());
  }

private:
  uint64_t seed_;
  const uint64_t gamma_;

  uint64_t next_seed() { return seed_ += gamma_; }

  static uint32_t mix32(uint64_t z) {
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9L;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebL;
    return z ^ (z >> 31);
  }

  static uint32_t mix64(uint64_t z) {
    z = (z ^ (z >> 33)) * 0xff51afd7ed558ccdL;
    z = (z ^ (z >> 33)) * 0xc4ceb9fe1a85ec53L;
    return z ^ (z >> 33);
  }

  static uint64_t mix64variant13(uint64_t z) {
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9L;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebL;
    return z ^ (z >> 31);
  }

  static uint64_t mix_gamma(uint64_t z) {
    z = mix64variant13(z) | 1L;
    int n = __builtin_popcountll(z ^ (z >> 1));
    if (n >= 24)
      z ^= 0xaaaaaaaaaaaaaaaaL;
    return z;
  }
};
} // namespace redstone::random