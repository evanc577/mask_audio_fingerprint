#include "database.hpp"

database::database(std::string filename) {
  rc = unqlite_open(&pDb, filename.c_str(), UNQLITE_OPEN_CREATE);
}

database::~database() { unqlite_close(pDb); }

std::vector<fp_data_t> database::get(uint32_t key) {
  std::vector<fp_data_t> ret;

  // get size of return value
  unqlite_int64 nBytes;
  rc = unqlite_kv_fetch(pDb, (void *)&key, sizeof(key), NULL, &nBytes);

  // return length 0 vector if key is not found
  if (rc != UNQLITE_OK) {
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

void database::put(uint32_t key, const fp_data_t& value) {
  // get current values
  auto v = get(key);

  // append new value
  v.emplace_back(value);

  // put value back into database
  rc = unqlite_kv_store(pDb, static_cast<void *>(&key), sizeof(key), v.data(),
                        v.size() * sizeof(fp_data_t));
}
