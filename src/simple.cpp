#include <cassert>
#include <cstdio>
#include <ctime>
#include <sqlite3.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  int s = socket(AF_INET, SOCK_DGRAM, 0);
  assert(s > 0);
  ::close(s);

  printf("hi!\n");
  return 0;

  struct timespec tm;
  clock_gettime(CLOCK_BOOTTIME, &tm);

  sqlite3 *db;
  int rc = sqlite3_open("sqlite.db", &db);
  if (rc < 0) {
    throw std::runtime_error{"whoops!"};
  }

  sqlite3_close(db);
}