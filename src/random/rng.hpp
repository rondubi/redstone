#pragma once

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace redstone::random {
template <typename Rng>
struct rng_traits {
  static constexpr size_t seed_size = Rng::seed_size;
};

template <typename Rng, typename Source>
Rng seed_from_rng(Source &source) {
  std::byte seed[rng_traits<Rng>::seed_size];
  source.fill_bytes(seed);
  return Rng{seed};
}

template <typename T>
struct sample_uniform {
  template <typename Rng>
  T operator()(Rng &&, const T &first, const T &last) = delete;
};

template <>
struct sample_uniform<uint64_t> {
  uint64_t first;
  uint64_t last;

  template <typename Rng>
  uint64_t operator()(Rng &&rng, uint64_t first, uint64_t last) {
    auto width = last - first;
    auto v = rng.next();
    return first + (v % width);
  }
};

template <>
struct sample_uniform<long> {
  long first;
  long last;

  template <typename Rng>
  long operator()(Rng &&rng, long first, long last) {
    return sample_uniform<uint64_t>{}(rng, first, last);
  }
};

template <typename Repr, typename Period>
struct sample_uniform<std::chrono::duration<Repr, Period>> {

  template <typename Rng>
  std::chrono::duration<Repr, Period>
  operator()(Rng &&rng, std::chrono::duration<Repr, Period> first,
             std::chrono::duration<Repr, Period> last) {
    auto v = sample_uniform<Repr>{}(rng, first.count(), last.count());
    return std::chrono::duration<Repr, Period>{v};
  }
};

template <typename T, typename Rng>
T gen_range(Rng &&rng, T first, T last) {
  return sample_uniform<T>{}(rng, first, last);
}

template <typename Rng>
bool gen_bool(Rng &&rng, double p) {
  assert(0.0 <= p && p <= 1.0);

  auto threshold = static_cast<uint64_t>(p * (2.0 * uint64_t(1llu << 63)));
  auto v = rng.next();

  if (threshold == std::numeric_limits<uint64_t>::max())
    return true;

  return v < threshold;
}
} // namespace redstone::random