#ifndef _FINGERPRINT_H
#define _FINGERPRINT_H

#include <array>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "kfr/base.hpp"
#include "kfr/dft.hpp"
#include "kfr/dsp.hpp"
#include "kfr/io.hpp"

#include "types.hpp"

class fingerprint {
public:
  // fingerprint();

  // fingerprint(const fingerprint &other);

  // ~fingerprint();

  std::vector<fp_t>
  get_fingerprints(std::vector<int16_t> buffer);

  static constexpr int FS = 4000;

private:
  static constexpr int WIN_STEP_MS = 10;
  static constexpr int WIN_SIZE_MS = 100;

  std::vector<kfr::univector<kfr::f64>>
  calc_stft(const kfr::univector<kfr::f64> &buffer);

  std::vector<kfr::univector<kfr::f64>>
  calc_mels(const std::vector<kfr::univector<kfr::f64>> &stft);

  std::vector<fp_t>
  calc_fingerprints(const std::vector<kfr::univector<kfr::f64>> &mels);
};

#endif
