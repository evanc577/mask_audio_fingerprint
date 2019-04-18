#include "fingerprint.hpp"

// calculate fingerprint for the current buffer
std::vector<fp_t>
fingerprint::get_fingerprints(const std::vector<double> &buffer) {
  kfr::univector<kfr::f64> kfr_buffer(buffer.begin(), buffer.end());

  return get_fingerprints(kfr_buffer);
}

// calculate fingerprint for the current buffer
std::vector<fp_t>
fingerprint::get_fingerprints(const kfr::univector<kfr::f64> &buffer) {
  auto stft = calc_stft(buffer);
  auto mels = calc_mels(stft);
  auto fingerprints = calc_fingerprints(mels);
  return fingerprints;
}
// calculate the spectrogram
std::vector<kfr::univector<kfr::f64>>
fingerprint::calc_stft(const kfr::univector<kfr::f64> &buffer) {
  // calculate constants
  constexpr int win_size = FS * WIN_SIZE_MS / 1000;
  constexpr int win_step = FS * WIN_STEP_MS / 1000;
  size_t num_frames = (buffer.size() - win_size) / win_step + 2;

  // create window
  auto window = kfr::window_hamming(win_size);

  // initialize stft 2d vector
  std::vector<kfr::univector<kfr::f64>> stft(num_frames);

  // process every window
  for (size_t i = 0; i < num_frames; ++i) {
    int begin_idx = i * win_step;

    // apply window
    kfr::univector<kfr::f64> slice = buffer.slice(begin_idx, win_size);
    slice.resize(win_size, 0);
    kfr::univector<kfr::f64> windowed = window * slice;

    // apply dft
    stft[i] = kfr::cabs(kfr::realdft(windowed));
  }

  return stft;
}

// calculate mel filterbank
std::vector<kfr::univector<kfr::f64>>
fingerprint::calc_mels(const std::vector<kfr::univector<kfr::f64>> &stft) {
  constexpr size_t num_bands = 18;
  std::vector<kfr::univector<kfr::f64>> mels(stft.size());
  for (auto &f : mels) {
    f.resize(num_bands);
  }

  // create filterbanks
  double low_mfreq = 0;
  double high_mfreq = 2595 * log10(1 + static_cast<double>(FS) / 2 / 700);
  auto m_points = kfr::linspace(low_mfreq, high_mfreq, num_bands + 2, true);
  kfr::univector<kfr::f64, num_bands + 2> hz_points =
      (700 * (kfr::exp10(m_points / 2595) - 1));

  kfr::univector<kfr::i32> bin =
      (FS * WIN_SIZE_MS / 1000.0 + 1) / FS * hz_points;
  std::vector<kfr::univector<kfr::f64>> f_bank(num_bands);

  // apply filterbank to each band
  for (size_t m = 1; m < num_bands + 1; ++m) {
    // create triangular windows
    size_t f_m_minus = bin[m - 1];
    size_t f_m = bin[m];
    size_t f_m_plus = bin[m + 1];

    f_bank[m - 1].resize(FS * WIN_SIZE_MS / 1000 / 2 + 1, 0);

    for (size_t k = f_m_minus; k < f_m; ++k) {
      f_bank[m - 1][k] =
          static_cast<kfr::f64>(k - bin[m - 1]) / (bin[m] - bin[m - 1]);
    }
    for (size_t k = f_m; k < f_m_plus; ++k) {
      f_bank[m - 1][k] =
          static_cast<kfr::f64>(bin[m + 1] - k) / (bin[m + 1] - bin[m]);
    }

    // apply triangular windows
    for (size_t i = 0; i < stft.size(); ++i) {
      mels[i][m - 1] = kfr::dotproduct(f_bank[m - 1], stft[i]);
    }
  }

  return mels;
}

// find peaks and calculate fingerprints
std::vector<fp_t> fingerprint::calc_fingerprints(
    const std::vector<kfr::univector<kfr::f64>> &mels) {

  std::vector<fp_t> fingerprints;

  for (size_t t = 9; t < mels.size() - 9; ++t) {
    for (size_t b = 1; b < mels[0].size() - 1; ++b) {
      // find local peaks
      if (mels[t][b] <= mels[t - 1][b] || mels[t][b] <= mels[t + 1][b]) {
        continue;
      }
      if (mels[t][b] <= mels[t][b - 1] || mels[t][b] <= mels[t][b + 1]) {
        continue;
      }

      // compute region values
      // region 1
      double r_1a, r_1b, r_1c, r_1d, r_1e, r_1f, r_1g, r_1h;
      r_1a = (mels[t - 9][b] + mels[t - 8][b] + mels[t - 7][b]) / 3;
      r_1b = (mels[t - 7][b] + mels[t - 6][b] + mels[t - 5][b]) / 3;
      r_1c = (mels[t - 5][b] + mels[t - 4][b] + mels[t - 3][b]) / 3;
      r_1d = (mels[t - 3][b] + mels[t - 2][b] + mels[t - 1][b]) / 3;
      r_1e = (mels[t + 1][b] + mels[t + 2][b] + mels[t + 3][b]) / 3;
      r_1f = (mels[t + 3][b] + mels[t + 4][b] + mels[t + 5][b]) / 3;
      r_1g = (mels[t + 5][b] + mels[t + 6][b] + mels[t + 7][b]) / 3;
      r_1h = (mels[t + 7][b] + mels[t + 8][b] + mels[t + 9][b]) / 3;

      // region 2
      double r_2a, r_2b, r_2c, r_2d;
      if (b == mels[0].size() - 2) {
        r_2a = (mels[t - 1][b + 1] + mels[t][b + 1] + mels[t + 1][b + 1]) / 3;
      } else {
        r_2a = (mels[t - 1][b + 2] + mels[t][b + 2] + mels[t + 1][b + 2]) / 3;
      }
      r_2b = (mels[t - 1][b + 1] + mels[t][b + 1] + mels[t + 1][b + 1]) / 3;
      r_2c = (mels[t - 1][b - 1] + mels[t][b - 1] + mels[t + 1][b - 1]) / 3;
      if (b == 1) {
        r_2d = (mels[t - 1][b - 1] + mels[t][b - 1] + mels[t + 1][b - 1]) / 3;
      } else {
        r_2d = (mels[t - 1][b - 2] + mels[t][b - 2] + mels[t + 1][b - 2]) / 3;
      }

      // region 3
      double r_3a, r_3b, r_3c, r_3d;
      r_3a = (mels[t - 2][b + 1] + mels[t - 1][b + 1] + mels[t][b + 1] +
              mels[t - 2][b] + mels[t - 1][b]) /
             5;
      r_3b = (mels[t][b + 1] + mels[t + 1][b + 1] + mels[t + 2][b + 1] +
              mels[t + 1][b] + mels[t + 2][b]) /
             5;
      r_3c = (mels[t + 1][b] + mels[t + 2][b] + mels[t][b - 1] +
              mels[t + 1][b - 1] + mels[t + 2][b - 1]) /
             5;
      r_3d = (mels[t - 2][b] + mels[t - 1][b] + mels[t - 1][b - 1] +
              mels[t - 1][b - 1] + mels[t][b - 1]) /
             5;

      // region 4
      double r_4a, r_4b, r_4c, r_4d, r_4e, r_4f, r_4g, r_4h;
      if (b == mels[0].size() - 2) {
        r_4a = (mels[t - 9][b + 1] + mels[t - 8][b + 1] + mels[t - 7][b + 1] +
                mels[t - 6][b + 1]) /
               4;
        r_4b = (mels[t - 5][b + 1] + mels[t - 4][b + 1] + mels[t - 3][b + 1] +
                mels[t - 2][b + 1]) /
               4;
        r_4e = (mels[t + 5][b + 1] + mels[t + 4][b + 1] + mels[t + 3][b + 1] +
                mels[t + 2][b + 1]) /
               4;
        r_4f = (mels[t + 9][b + 1] + mels[t + 8][b + 1] + mels[t + 7][b + 1] +
                mels[t + 6][b + 1]) /
               4;
      } else {
        r_4a = (mels[t - 9][b + 2] + mels[t - 8][b + 2] + mels[t - 7][b + 2] +
                mels[t - 6][b + 2] + mels[t - 9][b + 1] + mels[t - 8][b + 1] +
                mels[t - 7][b + 1] + mels[t - 6][b + 1]) /
               8;
        r_4b = (mels[t - 5][b + 2] + mels[t - 4][b + 2] + mels[t - 3][b + 2] +
                mels[t - 2][b + 2] + mels[t - 5][b + 1] + mels[t - 4][b + 1] +
                mels[t - 3][b + 1] + mels[t - 2][b + 1]) /
               8;
        r_4e = (mels[t + 5][b + 2] + mels[t + 4][b + 2] + mels[t + 3][b + 2] +
                mels[t + 2][b + 2] + mels[t + 5][b + 1] + mels[t + 4][b + 1] +
                mels[t + 3][b + 1] + mels[t + 2][b + 1]) /
               8;
        r_4f = (mels[t + 9][b + 2] + mels[t + 8][b + 2] + mels[t + 7][b + 2] +
                mels[t + 6][b + 2] + mels[t + 9][b + 1] + mels[t + 8][b + 1] +
                mels[t + 7][b + 1] + mels[t + 6][b + 1]) /
               8;
      }
      if (b == 1) {
        r_4c = (mels[t - 9][b - 1] + mels[t - 8][b - 1] + mels[t - 7][b - 1] +
                mels[t - 6][b - 1]) /
               4;
        r_4d = (mels[t - 5][b - 1] + mels[t - 4][b - 1] + mels[t - 3][b - 1] +
                mels[t - 2][b - 1]) /
               4;
        r_4g = (mels[t + 5][b - 1] + mels[t + 4][b - 1] + mels[t + 3][b - 1] +
                mels[t + 2][b - 1]) /
               4;
        r_4h = (mels[t + 9][b - 1] + mels[t + 8][b - 1] + mels[t + 7][b - 1] +
                mels[t + 6][b - 1]) /
               4;
      } else {
        r_4c = (mels[t - 9][b + 2] + mels[t - 8][b + 2] + mels[t - 7][b + 2] +
                mels[t - 6][b + 2] + mels[t - 9][b - 1] + mels[t - 8][b - 1] +
                mels[t - 7][b - 1] + mels[t - 6][b - 1]) /
               8;
        r_4d = (mels[t - 5][b + 2] + mels[t - 4][b + 2] + mels[t - 3][b + 2] +
                mels[t - 2][b + 2] + mels[t - 5][b - 1] + mels[t - 4][b - 1] +
                mels[t - 3][b - 1] + mels[t - 2][b - 1]) /
               8;
        r_4g = (mels[t + 5][b + 2] + mels[t + 4][b + 2] + mels[t + 3][b + 2] +
                mels[t + 2][b + 2] + mels[t + 5][b - 1] + mels[t + 4][b - 1] +
                mels[t + 3][b - 1] + mels[t + 2][b - 1]) /
               8;
        r_4h = (mels[t + 9][b + 2] + mels[t + 8][b + 2] + mels[t + 7][b + 2] +
                mels[t + 6][b + 2] + mels[t + 9][b - 1] + mels[t + 8][b - 1] +
                mels[t + 7][b - 1] + mels[t + 6][b - 1]) /
               8;
      }


      // compute mask
      std::array<double, 22> energy_diffs;

      // horizontal max
      energy_diffs[0] = r_1a - r_1b;
      energy_diffs[1] = r_1b - r_1c;
      energy_diffs[2] = r_1c - r_1d;
      energy_diffs[3] = r_1d - r_1e;
      energy_diffs[4] = r_1e - r_1f;
      energy_diffs[5] = r_1f - r_1g;
      energy_diffs[6] = r_1g - r_1h;

      // vertical max
      energy_diffs[7] = r_2a - r_2b;
      energy_diffs[8] = r_2b - r_2c;
      energy_diffs[9] = r_2c - r_2d;

      // intermediate quadrants
      energy_diffs[10] = r_3a - r_3b;
      energy_diffs[11] = r_3d - r_3c;
      energy_diffs[12] = r_3a - r_3d;
      energy_diffs[13] = r_3b - r_3c;

      // extended quadrants 1
      energy_diffs[14] = r_4a - r_4b;
      energy_diffs[15] = r_4c - r_4d;
      energy_diffs[16] = r_4e - r_4f;
      energy_diffs[17] = r_4g - r_4h;

      // extended quadrants 2
      energy_diffs[18] = (r_4a + r_4b) - (r_4c + r_4d);
      energy_diffs[19] = (r_4e + r_4f) - (r_4g + r_4h);
      energy_diffs[20] = (r_4c + r_4d) - (r_4e + r_4f);
      energy_diffs[21] = (r_4a + r_4b) - (r_4g + r_4h);


      // generate fingerprint
      uint32_t fp = 0;

      // mask bits
      for (size_t i = 0; i < energy_diffs.size(); ++i) {
        fp |= energy_diffs[i] > 0 ? (1 << i) : 0;
      }

      // band number
      fp |= (b - 1) << (energy_diffs.size());

      // add fingerprint to return vector
      fingerprints.emplace_back(fp, t);
    }
  }

  return fingerprints;
}
