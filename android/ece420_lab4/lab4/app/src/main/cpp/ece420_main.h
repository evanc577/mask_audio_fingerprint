//
// Created by daran on 1/12/2017 to be used in ECE420 Sp17 for the first time.
// Modified by dwang49 on 1/1/2018 to adapt to Android 7.0 and Shield Tablet updates.
//

#ifndef ECE420_MAIN_H
#define ECE420_MAIN_H

#include <array>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>

#include "audio_common.h"
#include "debug_utils.h"
#include "buf_manager.h"
#include "identify.h"

void ece420ProcessFrame(sample_buf *dataBuf);

void check_fingerprints();
std::vector<fp_data_t> find_matches(const std::vector<fp_t> &fingerprints, database &fp_db);

#endif //ECE420_MAIN_H
