#ifndef _PROCESS_H
#define _PROCESS_H

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdint>

#include "kfr/base.hpp"
#include "kfr/dsp.hpp"
#include "kfr/dft.hpp"
#include "kfr/io.hpp"


class fingerprint {
  public:
    fingerprint(int fs);
    fingerprint(const fingerprint& other);
    ~fingerprint();
    void process(const kfr::univector<kfr::f64>& data);
  private:
    static const int ARR_SIZE = 2000;
    static const int WIN_STEP_MS = 10;
    static const int WIN_SIZE_MS = 100;


    void process_full_buffer();
    void calc_fingerprint();
    std::vector<kfr::univector<kfr::f64>> calc_stft();
    std::vector<kfr::univector<kfr::f64>> calc_mels();

    kfr::univector<kfr::f64> _arr1;
    kfr::univector<kfr::f64> _arr2;

    kfr::univector<kfr::f64> _process_buf;

    int _cur_idx;
    bool doing_arr1;

    std::atomic<bool> _arr1_full;
    std::atomic<bool> _arr2_full;

    std::atomic<bool> _first;
    std::atomic<bool> _done;

    std::mutex _mtx;
    std::condition_variable _cv;

    int _fs_in;
    int _fs_out;
};

#endif
