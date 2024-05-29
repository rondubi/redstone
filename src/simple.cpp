#include <sqlite3.h>
#include <stdexcept>

int main() {
  sqlite3 *db;
  int rc = sqlite3_open("sqlite.db", &db);
  if (rc < 0) {
    throw std::runtime_error{"whoops!"};
  }

  sqlite3_close(db);
}