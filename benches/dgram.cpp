#include <atomic>
#include <cassert>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>

int main() {
  std::string msg;

  int s1 = socket(AF_UNIX, SOCK_DGRAM, 0);
  //   int s2 = socket(AF_UNIX, SOCK_DGRAM, 0);

  struct sockaddr_un saddr = {
      .sun_family = AF_UNIX,
  };
  strcpy(saddr.sun_path, "path");

  std::atomic_int remain = 1;

  std::atomic_bool ready = false;

  std::thread t1{
      [&msg, &ready, &remain]() {
        int s = socket(AF_UNIX, SOCK_DGRAM, 0);

        assert(0 > bind(s, (sockaddr *)&saddr, sizeof(saddr)));

        ready = true;

        for (;;) {
          struct sockaddr_storage storage;
          socklen_t len = sizeof(storage);

          std::string buf;
          buf.resize(msg.size());

          recvfrom(s, buf.data(), buf.size(), 0, (struct sockaddr *)&storage,
                   &len);

          if (remain == 0)
            break;
        }
      },
  };

  std::thread t2{
      [&msg, &ready, &remain, &saddr]() {
        int s = socket(AF_UNIX, SOCK_DGRAM, 0);

        while (!ready) {
        }

        for (;;) {
          struct sockaddr_storage storage;
          socklen_t len = sizeof(storage);

          std::string buf;
          buf.resize(msg.size());

          sendto(s, msg.data(), msg.size(), 0, (struct sockaddr *)&saddr,
                 sizeof(saddr));

          remain--;
          if (remain == 0)
            break;
        }
      },
  };

  t1.join();
  t2.join();
}