#ifndef _AUDIO_HELPER_H
#define _AUDIO_HELPER_H

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}


class audio_helper {
  public:
    std::vector<int16_t> read_from_file(const std::string filename);
    std::vector<int16_t> resample_4k(std::vector<int16_t> data, int fs);
};

#endif
