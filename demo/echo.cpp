#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "demo.h"

int main(int argc, char **argv) {
  auto start = std::chrono::steady_clock::now();

  int s = socket(AF_UNIX, SOCK_DGRAM, 0);
  check_return_value(s);

  auto addr = get_addr();
  int rc = bind(s, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
  check_return_value(rc);

  std::string buf;

  for (uint64_t i = 0; i < iters; ++i) {
    auto t = std::chrono::steady_clock::now() - start;
    auto t_s = std::chrono::duration_cast<std::chrono::seconds>(t);

    buf.resize(8192);

    struct sockaddr_storage storage;
    socklen_t len;

    auto ret =
        recvfrom(s, buf.data(), buf.size(), 0, (sockaddr *)&storage, &len);
    check_return_value(ret);
    buf.resize(ret);

    printf("received datagram: '%s'\n", buf.c_str());

    if (buf == "done") {
      break;
    }
  }
}
