#include "fingerprint.hpp"

// calculate fingerprint for the current buffer
std::vector<std::pair<uint32_t, int32_t>>
fingerprint::get_fingerprints(kfr::univector<kfr::f64> buffer) {
  auto stft = calc_stft(buffer);
  auto mels = calc_mels(stft);
  auto fingerprints = calc_fingerprints(mels);
  return fingerprints;
}

// calculate the spectrogram
std::vector<kfr::univector<kfr::f64>>
fingerprint::calc_stft(kfr::univector<kfr::f64> buffer) {
  // TODO: implement stft calculation
  static_cast<void>(buffer);
  std::vector<kfr::univector<kfr::f64>> stft;
  return stft;
}

// calculate mel filterbank
std::vector<kfr::univector<kfr::f64>>
fingerprint::calc_mels(std::vector<kfr::univector<kfr::f64>> stft) {
  // TODO: implement mel filterbank calculation
  static_cast<void>(stft);
  std::vector<kfr::univector<kfr::f64>> mels;
  return mels;
}

// find peaks and calculate fingerprints
std::vector<std::pair<uint32_t, int32_t>>
calc_fingerprints(std::vector<kfr::univector<kfr::f64>> mels) {
  // TODO: finish this function
  static_cast<void>(mels);
  std::vector<std::pair<uint32_t, int32_t>> fingerprints;
  return fingerprints;
}
