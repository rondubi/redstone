#include <ctime>
#include <sqlite3.h>
#include <stdexcept>

int main() {
  struct timespec tm;
  clock_gettime(CLOCK_BOOTTIME, &tm);

  sqlite3 *db;
  int rc = sqlite3_open("sqlite.db", &db);
  if (rc < 0) {
    throw std::runtime_error{"whoops!"};
  }

  sqlite3_close(db);
}