#include "identify.hpp"

int audio_callback(void *outputBuffer, void *inputBuffer,
                   const unsigned int nBufferFrames, double streamTime,
                   RtAudioStreamStatus status, void *userData) {
  static_cast<void>(outputBuffer);
  static_cast<void>(streamTime);
  static_cast<void>(status);
  static_cast<void>(userData);

  double *buf = (double *)inputBuffer;
  kfr::univector<kfr::f64> arr(nBufferFrames);
  for (size_t i = 0; i < nBufferFrames; ++i) {
    arr[i] = buf[i];
  }

  fill_double_bufs(arr);
  return 0;
}

void fill_double_bufs(const kfr::univector<kfr::f64> &data) {
  if (input_buf.size() != data.size()) {
    input_buf.resize(3 * data.size());
  }

  if (input_buf_n == 0) {
    std::copy(data.begin(), data.end(), input_buf.begin());
    ++input_buf_n;
    return;
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

  std::cout << "Listening" << std::flush;

  while (true) {
    // wait for one of the double buffers to fill
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck);
    lck.unlock();

    std::cout << "." << std::flush;

    // error condition
    if (buf_1_full && buf_2_full) {
      std::cerr << "BUFFERS FULL!" << std::endl;
      break;
    }

    // copy from double buffer to process buffer
    if (buf_1_full) {
      std::copy(buf_1.begin(), buf_1.end(), process_buf.begin());
    } else {
      std::copy(buf_2.begin(), buf_2.end(), process_buf.begin());
    }

    // calculate fingerprints
    auto fingerprints = fp.get_fingerprints(process_buf);

    std::cerr << fingerprints.size() << std::endl;

    // find matching fingerprints in database
    auto matches = find_matches(fingerprints);

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

    // show output information if match is found
    if (cur_max >= THRESHOLD) {
      auto result = songs_db.get_song(cur_max_id);
      int elapsed_time = (elapsed + cur_max_t) * 10 / 1000;
      int elapsed_min = elapsed_time / 60;
      int elapsed_sec = elapsed_time % 60;

      std::cout << std::endl;
      printf("result:       %s\n", result.c_str());
      printf("score:        %d\n", cur_max);
      printf("elapsed time: %02d:%02d\n", elapsed_min, elapsed_sec);
      return;
    }

    // check timeout
    if (elapsed * 10 / 1000 > TIMEOUT) {
      std::cout << std::endl;
      std::cout << "No matching song found" << std::endl;
      return;
    }

    // reset flags
    if (buf_1_full) {
      buf_1_full = false;
    } else {
      buf_2_full = false;
    }
  }
}

std::vector<fp_data_t> find_matches(const std::vector<fp_t> &fingerprints) {
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

int main() {
  RtAudio adc;
  if (adc.getDeviceCount() < 1) {
    std::cout << "\nNo audio devices found!\n";
    exit(0);
  }
  RtAudio::StreamParameters parameters;
  parameters.deviceId = adc.getDefaultInputDevice();
  parameters.nChannels = 1;
  parameters.firstChannel = 0;
  unsigned int buffer_frames = BUFFER_FRAMES;

  try {
    adc.openStream(NULL, &parameters, RTAUDIO_FLOAT64, SAMPLE_RATE,
                   &buffer_frames, &audio_callback);
    adc.startStream();
  } catch (RtAudioError &e) {
    e.printMessage();
    exit(0);
  }

  std::thread fp_listener(check_fingerprints);

  // wait for song to be identified
  fp_listener.join();

  try {
    // Stop the stream
    adc.stopStream();
  } catch (RtAudioError &e) {
    e.printMessage();
  }
  if (adc.isStreamOpen())
    adc.closeStream();
  return 0;
}
