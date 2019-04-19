#ifndef _DATABASE_H
#define _DATABASE_H

#include <iostream>
#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "types.hpp"

extern "C" {
#include "unqlite/unqlite.h"
}

class database {
  public:
    database(const std::string filename);
    ~database();
    std::vector<fp_data_t> get_fp(uint32_t key);
    void put_fp(uint32_t key, const fp_data_t& value);
    std::string get_song(std::array<unsigned char, 16> key);
    void put_song(std::array<unsigned char, 16> key, const std::string &value);
  private:
    unqlite *pDb;
    int rc;
};

#endif
