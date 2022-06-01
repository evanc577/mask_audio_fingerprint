#include "audio_helper.hpp"

std::vector<double> audio_helper::read_from_file(const std::string filename) {
  std::vector<double> ret(0);

  av_log_set_level(AV_LOG_QUIET);

  // get format from audio file
  AVFormatContext *format = avformat_alloc_context();
  if (avformat_open_input(&format, filename.c_str(), NULL, NULL) != 0) {
    return ret;
  }
  if (avformat_find_stream_info(format, NULL) < 0) {
    return ret;
  }

  // Find the index of the first audio stream
  int stream_index = -1;
  for (size_t i = 0; i < format->nb_streams; i++) {
    if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      stream_index = i;
      break;
    }
  }
  if (stream_index == -1) {
    return ret;
  }
  AVStream *stream = format->streams[stream_index];

  // find & open codec
  const AVCodec *c = avcodec_find_decoder(stream->codecpar->codec_id);
  AVCodecContext *codec = avcodec_alloc_context3(c);
  avcodec_parameters_to_context(codec, stream->codecpar);
  if (avcodec_open2(codec, c, NULL) < 0) {
    return ret;
  }

  // prepare resampler
  struct SwrContext *swr = swr_alloc();
  av_opt_set_int(swr, "in_channel_count", stream->codecpar->channels, 0);
  av_opt_set_int(swr, "out_channel_count", 1, 0);
  av_opt_set_int(swr, "in_channel_layout", stream->codecpar->channel_layout, 0);
  av_opt_set_int(swr, "out_channel_layout", AV_CH_LAYOUT_MONO, 0);
  av_opt_set_int(swr, "in_sample_rate", stream->codecpar->sample_rate, 0);
  av_opt_set_int(swr, "out_sample_rate", 4000, 0);
  av_opt_set_sample_fmt(swr, "in_sample_fmt",
                        (AVSampleFormat)stream->codecpar->format, 0);
  av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_DBL, 0);
  swr_init(swr);
  if (!swr_is_initialized(swr)) {
    return ret;
  }

  // prepare to read data
  AVPacket packet;
  av_init_packet(&packet);
  AVFrame *frame = av_frame_alloc();
  if (!frame) {
    return ret;
  }

  // iterate through frames
  while (av_read_frame(format, &packet) >= 0) {
    // decode one frame
    if (avcodec_send_packet(codec, &packet)) {
      continue;
    }
    if (avcodec_receive_frame(codec, frame)) {
      continue;
    }

    // resample frames
    double *buffer;
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
  avcodec_close(codec);
  avformat_close_input(&format);
  avformat_free_context(format);

  return ret;
}
