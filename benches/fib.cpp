#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

int main() {

  const auto start = std::chrono::steady_clock::now();

  uint64_t a = 1;
  uint64_t b = 1;

  uint64_t iters = 5'000'000'000;

  for (uint64_t i = 0; i < iters; ++i) {
    uint64_t c = a + b;
    a = b;
    b = c;
  }

  const auto end = std::chrono::steady_clock::now();

  std::cout << "took " << (end - start).count() << " to compute " << b
            << std::endl;

  //   for ()
  return 0;
}
// 1121621100
// 1157662211