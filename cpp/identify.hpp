#ifndef _IDENTIFY_H
#define _IDENTIFY_H

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

#include "database.hpp"
#include "fingerprint.hpp"
#include "RtAudio.h"
#include "kfr/base.hpp"
#include "kfr/dft.hpp"
#include "kfr/dsp.hpp"
#include "kfr/io.hpp"

static constexpr int THRESHOLD = 5;
static constexpr int TIMEOUT = 15;

static constexpr int BUF_SIZE = 2000;
static constexpr int SAMPLE_RATE = 48000;
static constexpr int BUFFER_FRAMES = 1200;

kfr::univector<kfr::f64, BUF_SIZE> buf_1;
kfr::univector<kfr::f64, BUF_SIZE> buf_2;
kfr::univector<kfr::f64> input_buf;
kfr::univector<kfr::f64, BUF_SIZE> process_buf;

std::mutex mtx;
std::condition_variable cv;

std::mutex done_mtx;
std::condition_variable done_cv;

std::atomic<bool> buf_1_full(false);
std::atomic<bool> buf_2_full(false);
std::atomic<bool> done(false);

bool doing_buf_1;
int cur_idx;
int input_buf_n = 0;

fingerprint fp;
database fp_db("fingerprints.db");
database songs_db("songs.db");

int audio_callback(void *outputBuffer, void *inputBuffer,
             const unsigned int nBufferFrames, double streamTime,
             RtAudioStreamStatus status, void *userData);

void fill_double_bufs(const kfr::univector<kfr::f64> &data);
void check_fingerprints();
std::vector<fp_data_t> find_matches(const std::vector<fp_t> &fingerprints);


#endif
