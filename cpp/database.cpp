#include "database.hpp"

database::database(const std::string filename) {
  rc = unqlite_open(&pDb, filename.c_str(), UNQLITE_OPEN_CREATE);
}

database::~database() {
  unqlite_lib_shutdown();
}

std::vector<fp_data_t> database::get_fp(uint32_t key) {
  std::vector<fp_data_t> ret;

  // get size of return value
  unqlite_int64 nBytes;
  rc = unqlite_kv_fetch(pDb, (void *)&key, sizeof(key), NULL, &nBytes);

  // return length 0 vector if key is not found
  if (rc != UNQLITE_OK || nBytes == 0) {
    ret.resize(0);
    return ret;
  }

  // write value to a vector if a key is found
  size_t vec_length = nBytes / sizeof(fp_data_t);
  ret.resize(vec_length);
  rc = unqlite_kv_fetch(pDb, static_cast<void *>(&key), sizeof(key), ret.data(),
                        &nBytes);

  return ret;
}

void database::put_fp(uint32_t key, const fp_data_t &value) {
  // get current values
  auto v = get_fp(key);

  // append new value
  v.push_back(value);

  // put value back into database
  rc = unqlite_kv_store(pDb, static_cast<void *>(&key), sizeof(key), v.data(),
                        v.size() * sizeof(fp_data_t));
}

std::string database::get_song(std::array<unsigned char, 16> key) {
  std::string ret = "";

  // get size of return value
  unqlite_int64 nBytes;
  rc =
      unqlite_kv_fetch(pDb, static_cast<void *>(key.data()), 16, NULL, &nBytes);
  if (rc != UNQLITE_OK || nBytes == 0) {
    return ret;
  }

  std::vector<uint8_t> temp(nBytes);
  ret.resize(nBytes);

  // write value to a vector if a key is found
  rc = unqlite_kv_fetch(pDb, static_cast<void *>(key.data()), 16, temp.data(),
                        &nBytes);

  std::copy(temp.begin(), temp.end(), ret.begin());
  return ret;
}

void database::put_song(std::array<unsigned char, 16> key,
                        const std::string &value) {
  // put value back into database
  rc = unqlite_kv_store(pDb, static_cast<void *>(key.data()), 16, value.c_str(),
                        value.length());
}
