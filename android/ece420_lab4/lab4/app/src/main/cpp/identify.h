//
// Created by evan on 4/18/19.
//

#ifndef LAB4_IDENTIFY_H
#define LAB4_IDENTIFY_H

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <map>
#include <thread>
#include <vector>
#include <string>

#include "database.hpp"
#include "fingerprint.hpp"
#include "kfr/base.hpp"
#include "kfr/dft.hpp"
#include "kfr/dsp.hpp"
#include "kfr/io.hpp"

static constexpr int THRESHOLD = 6;
static constexpr int TIMEOUT = 15;
static constexpr int THRESHOLD_2 = 9;
static constexpr int TIMEOUT_2 = 30;

static constexpr int BUF_SIZE = 2000;
static constexpr int SAMPLE_RATE = 48000;
static constexpr int BUFFER_FRAMES = 4800;

static kfr::univector<kfr::f64, BUF_SIZE> buf_1;
static kfr::univector<kfr::f64, BUF_SIZE> buf_2;
static kfr::univector<kfr::f64, 3*BUFFER_FRAMES> input_buf;
static kfr::univector<kfr::f64, BUF_SIZE> process_buf;

static std::mutex mtx;
static std::condition_variable cv;

static std::atomic<bool> buf_1_full(false);
static std::atomic<bool> buf_2_full(false);
static std::atomic<bool> done(false);

static bool doing_buf_1 = true;
static int cur_idx = 0;
static int input_buf_n = 0;

static fingerprint fp;

static std::string dir_path;

static bool initial = true;
static bool timeout = false;
static bool found = false;
static int elapsed_time;
static int elapsed_min;
static int elapsed_sec;



#endif //LAB4_IDENTIFY_H
