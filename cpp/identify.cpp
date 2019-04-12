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

void fill_double_bufs(const kfr::univector<kfr::f32> &data) {
  if (input_buf.size() != data.size()) {
    input_buf.resize(3 * data.size());
  }

  if (input_buf_n == 0) {
    std::copy(data.begin(), data.end(), input_buf.begin());
    ++input_buf_n;
  } else if (input_buf_n == 1) {
    std::copy(data.begin(), data.end(), input_buf.begin() + data.size());
    ++input_buf_n;
  } else if (input_buf_n == 2) {
    std::copy(data.begin(), data.end(), input_buf.begin() + 2 * data.size());
    ++input_buf_n;
  }

  // resample to 4 kHz
  auto r =
      kfr::resampler<kfr::f32>(kfr::resample_quality::low, fp.FS, SAMPLE_RATE);
  kfr::univector<kfr::f32> temp(data.size() * fp.FS / SAMPLE_RATE +
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
  while (true) {
    // wait for one of the double buffers to fill
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck);
    lck.unlock();

    // error condition
    if (buf_1_full && buf_2_full) {
      std::cerr << "BUFFERS FULL!" << std::endl;
      break;
    }

    // copy from double buffer to process buffer
    if (buf_1_full) {
      std::copy(process_buf.begin() + BUF_SIZE, process_buf.end(),
                process_buf.begin());
      std::copy(buf_1.begin(), buf_1.end(), process_buf.begin() + BUF_SIZE);
    } else {
      std::copy(process_buf.begin() + BUF_SIZE, process_buf.end(),
                process_buf.begin());
      std::copy(buf_2.begin(), buf_2.end(), process_buf.begin() + BUF_SIZE);
    }

    // calculate fingerprints
    auto fingerprints = fp.get_fingerprints(process_buf);

    // find matching fingerprints in database
    // TODO

    // compute histograms
    // TODO

    // check threshold
    // TODO
    bool found_match = false;

    // show output information if match is found
    // TODO
    if (found_match) {
      break;
    }

    // check elapsed time
    // TODO

    // reset flags
    if (buf_1_full) {
      buf_1_full = false;
    } else {
      buf_2_full = false;
    }
  }
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
