#include "fingerprint.hpp"

fingerprint::fingerprint(int fs) {
  _arr1.resize(ARR_SIZE);
  _arr2.resize(ARR_SIZE);
  _process_buf.resize(2 * ARR_SIZE);
  _arr1_full = false;
  _arr2_full = false;
  _first = true;
  _done = false;
  _cur_idx = 0;
  _fs_in = fs;
  _fs_out = 4000;
  doing_arr1 = true;

  std::thread buf_thread(&fingerprint::process_full_buffer, this);
  buf_thread.detach();
}

fingerprint::~fingerprint() {
  _done = true;
  _cv.notify_all();
}

fingerprint::fingerprint(const fingerprint &other) { return; }

void fingerprint::process(const kfr::univector<kfr::f64> &data) {
  // resmaple to 4 kHz
  auto r = kfr::resampler<kfr::f64>(kfr::resample_quality::normal, _fs_out,
                                      _fs_in);
  kfr::univector<kfr::f64> temp(data.size() * _fs_out / _fs_in +
                                  r.get_delay());
  r.process(temp, data);

  // write resampled signal to the double buffers
  for (auto it = temp.begin() + r.get_delay(); it < temp.end(); ++it) {
    if (doing_arr1 && !_arr1_full) {
      _arr1[_cur_idx] = *it;
      if (++_cur_idx == ARR_SIZE) {
        _arr1_full = true;
        doing_arr1 = false;
        _cur_idx = 0;
        _cv.notify_all();
      }
    } else if (!doing_arr1 && !_arr2_full) {
      _arr2[_cur_idx] = *it;
      if (++_cur_idx == ARR_SIZE) {
        _arr2_full = true;
        doing_arr1 = true;
        _cur_idx = 0;
        _cv.notify_all();
      }
    } else {
      std::cerr << "buffers full!" << std::endl;
    }
  }

  return;
}

void fingerprint::process_full_buffer() {
  while (true) {
    std::unique_lock<std::mutex> lck(_mtx);
    // wait for one of the double buffers to fill
    _cv.wait(lck);
    lck.unlock();

    if (_done) {
      break;
    }

    // copy the completed buffer and start processing
    if (_arr1_full) {
      std::copy(_process_buf.begin() + ARR_SIZE, _process_buf.end(),
                _process_buf.begin());
      std::copy(_arr1.begin(), _arr1.end(), _process_buf.begin() + ARR_SIZE);
      calc_fingerprint();
      std::cout << "processed 1" << std::endl;
      _arr1_full = false;
    }

    if (_arr2_full) {
      std::copy(_process_buf.begin() + ARR_SIZE, _process_buf.end(),
                _process_buf.begin());
      std::copy(_arr2.begin(), _arr2.end(), _process_buf.begin() + ARR_SIZE);
      calc_fingerprint();
      std::cout << "processed 2" << std::endl;
      _arr2_full = false;
    }
  }
}

// calculate fingerprint for the current buffer
void fingerprint::calc_fingerprint() {
  if (_first) {
    _first = false;
    return;
  }

  auto stft = calc_stft();
}

// calculate the spectrogram
std::vector<kfr::univector<kfr::f64>> fingerprint::calc_stft() {
  int win_size = _fs_out * WIN_SIZE_MS / 1000;
  int win_step = _fs_out * WIN_STEP_MS / 1000;
  int num_frames = (ARR_SIZE - win_size) / win_step + 2;

  auto window = kfr::window_hamming<kfr::f64>(win_size);
  std::vector<kfr::univector<kfr::f64>> stft(num_frames);

  for (int i = 0; i < num_frames; ++i) {
    int begin_idx = (i - 1) * win_step + ARR_SIZE;

    kfr::univector<kfr::f64> windowed =
        window * _process_buf.slice(begin_idx, win_size);

    stft[i] = kfr::cabs(kfr::realdft(windowed));
  }

  return stft;
}
