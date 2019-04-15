#include "audio_helper.hpp"

std::vector<int16_t> audio_helper::read_from_file(const std::string filename) {
  std::vector<int16_t> ret(0);

  // get format from audio file
  AVFormatContext *format = avformat_alloc_context();
  if (avformat_open_input(&format, filename.c_str(), NULL, NULL) != 0) {
    std::cerr << "Could not open file " << filename << std::endl;
    return ret;
  }
  if (avformat_find_stream_info(format, NULL) < 0) {
    std::cerr << "Could not retrieve stream info for file " << filename
              << std::endl;
    return ret;
  }

  // Find the index of the first audio stream
  int stream_index = -1;
  for (int i = 0; i < format->nb_streams; i++) {
    if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      stream_index = i;
      break;
    }
  }
  if (stream_index == -1) {
    std::cerr << "Could not retrieve audio stream info for file " << filename
              << std::endl;
    return ret;
  }
  AVStream *stream = format->streams[stream_index];

  // find & open codec
  AVCodecParameters *codecpar = stream->codecpar;
  AVCodec *codec = NULL;
  codec = avcodec_find_decoder(codecpar->codec_id);
  if (!codec) {
    std::cerr << "Codec not found for file " << filename << std::endl;
    return ret;
  }

  AVCodecContext *codec_ctx = NULL;
  codec_ctx = avcodec_alloc_context3(codec);
  if (!codec_ctx) {
    std::cerr << "Could not allocate audio codec context for file " << filename
              << std::endl;
    return ret;
  }
  codec_ctx->channels = codecpar->channels;
  codec_ctx->channel_layout = codecpar->channel_layout;
  codec_ctx->sample_rate = codecpar->sample_rate;
  codec_ctx->sample_rate = static_cast<AVSampleFormat>(codecpar->format);

  /* open it */
  if (avcodec_open2(codec_ctx, codec, NULL)) {
    std::cerr << "Could not open codec for file " << filename << std::endl;
    return ret;
  }

  // prepare to read data
  AVPacket packet;
  av_init_packet(&packet);
  AVFrame *frame = av_frame_alloc();
  if (!frame) {
    std::cerr << "Error allocating the frame for file " << filename
              << std::endl;
    return ret;
  }

  // prepare resampler
  struct SwrContext *swr = swr_alloc();
  av_opt_set_int(swr, "in_channel_count", codecpar->channels, 0);
  av_opt_set_int(swr, "out_channel_count", 1, 0);
  av_opt_set_int(swr, "in_channel_layout", codecpar->channel_layout, 0);
  av_opt_set_int(swr, "out_channel_layout", AV_CH_LAYOUT_MONO, 0);
  av_opt_set_int(swr, "in_sample_rate", codecpar->sample_rate, 0);
  av_opt_set_int(swr, "out_sample_rate", 4000, 0);
  av_opt_set_sample_fmt(swr, "in_sample_fmt",
                        static_cast<AVSampleFormat>(codecpar->format), 0);
  av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
  swr_init(swr);
  if (!swr_is_initialized(swr)) {
    std::cerr << "Resampler has not been properly initialized" << std::endl;
    return ret;
  }

  // iterate through frames
  while (av_read_frame(format, &packet) >= 0) {
    // decode one frame
    if (avcodec_send_packet(codec_ctx, &packet)) {
      continue;
    }
    if (avcodec_receive_frame(codec_ctx, frame)) {
      continue;
    }

    // resample frames
    int16_t *buffer;
    av_samples_alloc((uint8_t **)&buffer, NULL, 1, frame->nb_samples,
                     AV_SAMPLE_FMT_S16, 0);
    int frame_count =
        swr_convert(swr, (uint8_t **)&buffer, frame->nb_samples,
                    (const uint8_t **)frame->data, frame->nb_samples);

    // append resampled frames to data
    ret.insert(ret.end(), buffer, buffer + frame_count);
    av_free(buffer);
    av_packet_unref(&packet);
  }

  // clean up
  av_frame_free(&frame);
  swr_free(&swr);
  avcodec_close(codec_ctx);
  avcodec_free_context(&codec_ctx);
  avformat_close_input(&format);
  avformat_free_context(format);

  return ret;
}

std::vector<int16_t> audio_helper::resample_4k(std::vector<int16_t> data,
                                               int fs) {
  struct SwrContext *swr = swr_alloc();
  av_opt_set_int(swr, "in_channel_count", 1, 0);
  av_opt_set_int(swr, "out_channel_count", 1, 0);
  av_opt_set_int(swr, "in_channel_layout", AV_CH_LAYOUT_MONO, 0);
  av_opt_set_int(swr, "out_channel_layout", AV_CH_LAYOUT_MONO, 0);
  av_opt_set_int(swr, "in_sample_rate", fs, 0);
  av_opt_set_int(swr, "out_sample_rate", 4000, 0);
  av_opt_set_sample_fmt(swr, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
  av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
  swr_init(swr);

  std::vector<int16_t> ret(0);
  return ret;
}
