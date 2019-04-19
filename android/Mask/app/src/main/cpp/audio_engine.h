//
// Created by evan on 4/18/19.
//

#ifndef MASK_AUDIO_ENGINE_H
#define MASK_AUDIO_ENGINE_H

#include <oboe/Oboe.h>
#include "../../../../../oboe/src/common/OboeDebug.h"

class AudioEngine : public oboe::AudioStreamCallback {
public:
    void start();
    oboe::DataCallbackResult
    onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames);

private:
    int counter;
    oboe::AudioStream *stream;
};

#endif //MASK_AUDIO_ENGINE_H
