#ifndef _FINGERPRINT_H
#define _FINGERPRINT_H

#include <unordered_map>
#include <vector>

#include "kfr/base.hpp"
#include "kfr/dft.hpp"
#include "kfr/dsp.hpp"
#include "kfr/io.hpp"

class fingerprint {
public:
  fingerprint();

  fingerprint(const fingerprint &other);

  ~fingerprint();

  std::vector<std::pair<uint32_t, int32_t>>
  get_fingerprints(kfr::univector<kfr::f64> buffer);

  static constexpr int FS = 4000;

private:
  static constexpr int WIN_STEP_MS = 10;
  static constexpr int WIN_SIZE_MS = 100;

  std::vector<kfr::univector<kfr::f64>>
  calc_stft(kfr::univector<kfr::f64> buffer);

  std::vector<kfr::univector<kfr::f64>>
  calc_mels(std::vector<kfr::univector<kfr::f64>> stft);

  std::vector<std::pair<uint32_t, int32_t>>
  calc_fingerprints(std::vector<kfr::univector<kfr::f64>> mels);
};

#endif
