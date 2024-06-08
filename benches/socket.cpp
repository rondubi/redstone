#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  const uint64_t iters = 1'000'000;

  const auto start = std::chrono::steady_clock::now();

  for (uint64_t i = 0; i < iters; ++i) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    close(s);
  }

  const auto end = std::chrono::steady_clock::now();

  std::cout << "took " << std::chrono::nanoseconds(end - start).count() / iters
            << "ns per iteration\n";

  return 0;
}
// 1121621100
// 1157662211