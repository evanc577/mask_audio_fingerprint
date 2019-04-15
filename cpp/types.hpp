#ifndef _TYPES_H
#define _TYPES_H

#include <cstdint>

struct fp_t {
  uint32_t fp;
  int32_t t;

  fp_t(uint32_t fp, int32_t t) : fp(fp), t(t) {}
};

struct fp_data_t {
  uint8_t id[16];
  int32_t t;
};

#endif
