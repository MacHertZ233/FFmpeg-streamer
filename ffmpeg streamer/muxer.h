#pragma once

#include <QObject>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixfmt.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}
class Muxer:public QObject
{
    Q_OBJECT
public:
    Muxer(AVCodecContext* ctx_video, AVCodecContext* ctx_audio, const char* address);
    ~Muxer();

private:
    AVFormatContext* output;
    AVCodecContext* codec_context_video;
    AVCodecContext* codec_context_audio;
    uint64_t frame_index_video = 0;
    uint64_t frame_index_audio = 0;
    int64_t start_time = -1;

public slots:
    void slotWritePacket(AVPacket* packet);
};

