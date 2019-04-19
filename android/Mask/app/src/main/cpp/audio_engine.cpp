//
// Created by evan on 4/18/19.
//

#include "audio_engine.h"

void AudioEngine::start() {
    counter = 0;

    // make audio stream builder
    oboe::AudioStreamBuilder builder;
    builder.setCallback(this);
    builder.setDirection(oboe::Direction::Output);
    builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder.setSharingMode(oboe::SharingMode::Exclusive);

    // open stream
    oboe::Result r = builder.openStream(&stream);
    if (r != oboe::Result::OK) {
        LOGE("Error opening stream: %s", oboe::convertToText(r));
    }

    stream->setBufferSizeInFrames(stream->getFramesPerBurst() * 2);

    // start stream
    r = stream->requestStart();
    if (r != oboe::Result::OK) {
        LOGE("Error starting stream: %s", oboe::convertToText(r));
    }

    return;
}


oboe::DataCallbackResult
AudioEngine::onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) {
    return oboe::DataCallbackResult::Continue;
}
