#include <cstdint>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "demo.h"

int main() {
  int s = socket(AF_UNIX, SOCK_DGRAM, 0);
  check_return_value(s);

  const auto addr = get_addr();
  const int flags = 0;

  int64_t i = 0;

  for (;;) {
    std::string msg = "hello from message ";
    msg += std::to_string(i);
    msg += "!";

    int rc = sendto(s, msg.data(), msg.size(), flags,
                    reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
    if (rc < 0) {
      break;
    }
    // check_return_value(rc);

    if (rc == 0) {
      break;
    }

    sleep(1);

    i++;
  }
}
