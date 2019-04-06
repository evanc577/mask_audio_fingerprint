#include "RtAudio.h"
#include <vector>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <condition_variable>
#include <mutex>
#include "fingerprint.hpp"
#include "kfr/base.hpp"
#include "kfr/dsp.hpp"

static std::mutex mtx;
static std::condition_variable cv;
fingerprint fp = fingerprint(48000);


int record(void *outputBuffer, void *inputBuffer, const unsigned int nBufferFrames,
           double streamTime, RtAudioStreamStatus status, void *userData) {
  if (true) {
    double* buf = (double*)inputBuffer;
    kfr::univector<kfr::f64> arr(nBufferFrames);
    for (int i = 0; i < nBufferFrames; ++i) {
      arr[i] = buf[i];
    }
    fp.process(arr);
  } else {
    cv.notify_all();
  }

  return 0;
}


int main() {
  RtAudio adc;
  if (adc.getDeviceCount() < 1) {
    std::cout << "\nNo audio devices found!\n";
    exit(0);
  }
  RtAudio::StreamParameters parameters;
  parameters.deviceId = adc.getDefaultInputDevice();
  parameters.nChannels = 2;
  parameters.firstChannel = 0;
  unsigned int sampleRate = 48000;
  unsigned int bufferFrames = 1200;

  try {
    adc.openStream(NULL, &parameters, RTAUDIO_FLOAT64, sampleRate, &bufferFrames,
                   &record);
    adc.startStream();
  } catch (RtAudioError &e) {
    e.printMessage();
    exit(0);
  }

  std::unique_lock<std::mutex> lck(mtx);
  cv.wait(lck);

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
