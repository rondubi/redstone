#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>
#include <utility>

namespace redstone::random {
class xoshiro256_star_star {
public:
  static constexpr size_t seed_size = 32;

  xoshiro256_star_star() = default;

  explicit xoshiro256_star_star(std::span<const std::byte, seed_size> seed) {
    std::memcpy(state_, seed.data(), seed.size_bytes());
  }

  uint64_t next() {
    const uint64_t result = std::rotl(state_[1] * 5, 7) * 9;

    const uint64_t t = state_[1] << 17;

    state_[2] ^= state_[0];
    state_[3] ^= state_[1];
    state_[1] ^= state_[2];
    state_[0] ^= state_[3];

    state_[2] ^= t;

    state_[3] = std::rotl(state_[3], 45);

    return result;
  }

  void fill_bytes(std::span<std::byte> buf) {
    while (8 <= buf.size_bytes()) {
      uint64_t block = next();
      std::memcpy(buf.data(), &block, sizeof(block));
      buf = buf.subspan(8);
    }

    uint64_t block = next();
    std::memcpy(buf.data(), &block, buf.size_bytes());
  }

  void set_seed(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    state_[0] = a;
    state_[1] = b;
    state_[2] = c;
    state_[3] = d;
  }

  template <typename Rng>
  void seed_from(Rng &rng) {
    static_assert(
        std::is_same_v<decltype(std::declval<Rng &>().next()), uint64_t>);

    state_[0] = rng.next();
    state_[1] = rng.next();
    state_[2] = rng.next();
    state_[3] = rng.next();
  }

private:
  uint64_t state_[4]{};
};
} // namespace redstone::random