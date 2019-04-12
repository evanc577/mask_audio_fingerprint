#ifndef _DATABASE_H
#define _DATABASE_H

#include <cstdint>
#include <string>
#include <vector>
#include "unqlite/unqlite.h"

struct fp_data_t {
  uint8_t id[16];
  uint32_t t;
};

class database {
  public:
    database(std::string filename);
    ~database();
    std::vector<fp_data_t> get(uint32_t key);
    void put(uint32_t key, const fp_data_t& value);
  private:
    unqlite *pDb;
    int rc;
};

#endif
