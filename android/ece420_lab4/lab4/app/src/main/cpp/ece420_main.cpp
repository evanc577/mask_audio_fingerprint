//
// Created by daran on 1/12/2017 to be used in ECE420 Sp17 for the first time.
// Modified by dwang49 on 1/1/2018 to adapt to Android 7.0 and Shield Tablet updates.
//

#include "ece420_main.h"
#include "ece420_lib.h"
#include "kiss_fft/kiss_fft.h"

// JNI Function
extern "C" {
JNIEXPORT jstring JNICALL
Java_com_ece420_lab4_MainActivity_getMaskText(JNIEnv *env, jobject);

JNIEXPORT jint JNICALL
Java_com_ece420_lab4_MainActivity_getMaskStatus(JNIEnv *env, jobject);

JNIEXPORT jint JNICALL
Java_com_ece420_lab4_MainActivity_getMaskTime(JNIEnv *env, jobject);

JNIEXPORT void JNICALL
Java_com_ece420_lab4_MainActivity_initIdentify(JNIEnv *env, jclass, jstring path);

JNIEXPORT void JNICALL
Java_com_ece420_lab4_MainActivity_deleteIdentify(JNIEnv *env, jclass);
}

// Student Variables
int counter = 0;
std::string status = "";

void ece420ProcessFrame(sample_buf *dataBuf) {
    int16_t *int_buf = reinterpret_cast<int16_t *>(dataBuf->buf_);
    std::vector<double> data(dataBuf->size_ / sizeof(int16_t));
    for (int i = 0; i < dataBuf->size_; ++i) {
        data[i] = static_cast<double>(int_buf[i]);
    }

    if (input_buf_n == 0) {
        std::copy(data.begin(), data.end(), input_buf.begin()); ++input_buf_n; return;
    } else if (input_buf_n == 1) {
        std::copy(data.begin(), data.end(), input_buf.begin() + data.size());
        ++input_buf_n;
        return;
    } else if (input_buf_n == 2) {
        std::copy(data.begin(), data.end(), input_buf.begin() + 2 * data.size());
        ++input_buf_n;
        return;
    } else {
        std::copy(input_buf.begin() + data.size(), input_buf.end(), input_buf.begin());
        std::copy(data.begin(), data.end(), input_buf.begin() + 2 * data.size());
    }


    // resample to 4 kHz
    auto r =
            kfr::resampler<kfr::f64>(kfr::resample_quality::normal, fp.FS, SAMPLE_RATE);
    kfr::univector<kfr::f64> temp(input_buf.size() * fp.FS / SAMPLE_RATE +
                                  r.get_delay());
    r.process(temp, input_buf);

    // write resampled signal to the double buffers
    auto begin = temp.begin() + data.size() * fp.FS / SAMPLE_RATE + r.get_delay();
    auto end = begin + data.size() * fp.FS / SAMPLE_RATE;

    for (auto it = begin; it != end; ++it) {

        if (doing_buf_1 && !buf_1_full) {
            buf_1[cur_idx] = *it;
            if (++cur_idx == BUF_SIZE) {
                buf_1_full = true;
                doing_buf_1 = false;
                cur_idx = 0;
                cv.notify_all();
            }
        } else if (!doing_buf_1 && !buf_1_full) {
            buf_2[cur_idx] = *it;
            if (++cur_idx == BUF_SIZE) {
                buf_2_full = true;
                doing_buf_1 = true;
                cur_idx = 0;
                cv.notify_all();
            }
        } else {
            cv.notify_all();
        }
    }

    return;
}

void check_fingerprints() {
    std::map<std::array<uint8_t, 16>, std::map<int, int>> hists;
    int cur_max = 0;
    int cur_max_t = 0;
    std::array<uint8_t, 16> cur_max_id;
    int elapsed = 0;

    database fp_db(dir_path + "/fingerprints.db");
    database songs_db(dir_path + "/songs.db");

    while (true) {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck);
        lck.unlock();

        if (done) {
            status = "done";
            return;
        }

        // error condition
        if (buf_1_full && buf_2_full) {
            status = "buffers full";
            return;
        }

        // copy from double buffer to process buffer
        if (buf_1_full) {
            std::copy(buf_1.begin(), buf_1.end(), process_buf.begin());
        } else {
            std::copy(buf_2.begin(), buf_2.end(), process_buf.begin());
        }


        // calculate fingerprints
        auto fingerprints = fp.get_fingerprints(process_buf);

        // find matching fingerprints in database
        auto matches = find_matches(fingerprints, fp_db);

        // update elapsed time
        elapsed += BUF_SIZE * 100 / fp.FS;

        // compute histograms
        std::array<uint8_t, 16> temp_id;
        for (const auto &m : matches) {
            int dt = m.t - elapsed;
            std::copy(m.id, m.id + 16, temp_id.begin());

            // find histogram for id
            auto it = hists.find(temp_id);
            if (it == hists.end()) {
                std::map<int, int> temp_t;
                hists[temp_id] = temp_t;
            }

            // insert delta time
            auto it2 = hists[temp_id].find(dt);
            if (it2 == hists[temp_id].end()) {
                hists[temp_id][dt] = 0;
            }
            ++hists[temp_id][dt];


            // update current max score
            if (hists[temp_id][dt] > cur_max) {
                cur_max = hists[temp_id][dt];
                cur_max_t = dt;
                std::copy(temp_id.begin(), temp_id.end(), cur_max_id.begin());
            }
        }

        ++counter;
        status = std::to_string(cur_max);

        // show output information if match is found
        if (cur_max >= THRESHOLD) {
            auto result = songs_db.get_song(cur_max_id);
            elapsed_time = (elapsed + cur_max_t) * 10;
            elapsed_min = elapsed_time / 1000 / 60;
            elapsed_sec = elapsed_time / 1000 % 60;
            (void)elapsed_time;
            (void)elapsed_min;
            (void)elapsed_sec;

            status = result;
            found = true;
            return;
        }

        // check timeout
        if (elapsed * 10 / 1000 > TIMEOUT) {
            status = "timeout";
            timeout = true;
            return;
        }

        // reset flags
        if (buf_1_full) {
            buf_1_full = false;
        } else {
            buf_2_full = false;
        }

        if (done) {
            status = "done";
            return;
        }
    }
}

std::vector<fp_data_t> find_matches(const std::vector<fp_t> &fingerprints, database &fp_db) {
    std::vector<fp_data_t> all_matches;
    int num_matches = 0;
    for (fp_t f : fingerprints) {
        uint32_t fp = f.fp;
        for (int idx_1 = -1; idx_1 < 22; ++idx_1) {
            for (int idx_2 = -1; idx_2 < idx_1; ++idx_2) {
                uint32_t temp_fp = fp;
                if (idx_1 != -1) {
                    temp_fp ^= 1 << idx_1;
                }
                if (idx_2 != -1) {
                    temp_fp ^= 1 << idx_2;
                }

                auto matches = fp_db.get_fp(temp_fp);
                for (auto &m : matches) {
                    m.t -= f.t;
                }
                all_matches.insert(all_matches.end(), matches.begin(), matches.end());
                num_matches += matches.size();
            }
        }
    }
    return all_matches;
}


extern "C" {

JNIEXPORT jstring JNICALL
Java_com_ece420_lab4_MainActivity_getMaskText(JNIEnv *env, jobject obj) {
    std::string ret;
    if (found) {
        ret = status;
    } else if (timeout) {
        ret = "Timed out";
    } else if (initial || done) {
        ret = "Press start to identify";
    } else {
        ret = "Listening...";
    }
    return env->NewStringUTF(ret.c_str());
}

JNIEXPORT jint JNICALL
Java_com_ece420_lab4_MainActivity_getMaskStatus(JNIEnv *env, jobject obj) {
    if (found) {
        return 1;
    } else if (timeout) {
        return 2;
    } else {
        return 0;
    }
}

JNIEXPORT jint JNICALL
Java_com_ece420_lab4_MainActivity_getMaskTime(JNIEnv *env, jobject obj) {
    return elapsed_time;
}

JNIEXPORT void JNICALL
Java_com_ece420_lab4_MainActivity_initIdentify(JNIEnv *env, jclass, jstring path) {
    initial = false;
    counter = 0;
    status = "";
    buf_1_full = false;
    buf_2_full = false;
    done = false;
    doing_buf_1 = true;
    cur_idx = 0;
    input_buf_n = 0;
    timeout = false;
    found = false;
    const char *native_path = env->GetStringUTFChars(path, NULL);
    dir_path = native_path;
    env->ReleaseStringUTFChars(path, native_path);

    std::thread id_thread(check_fingerprints);
    id_thread.detach();

}

JNIEXPORT void JNICALL
Java_com_ece420_lab4_MainActivity_deleteIdentify(JNIEnv *env, jclass) {
    done = true;
    cv.notify_all();
}

}
