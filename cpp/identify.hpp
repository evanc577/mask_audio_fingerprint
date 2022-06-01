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

int audio_callback(void *outputBuffer, void *inputBuffer,
             const unsigned int nBufferFrames, double streamTime,
             RtAudioStreamStatus status, void *userData);

void fill_double_bufs(const kfr::univector<kfr::f64> &data);
void check_fingerprints();
std::vector<fp_data_t> find_matches(const std::vector<fp_t> &fingerprints);


#endif
