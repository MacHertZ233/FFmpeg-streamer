#pragma once

#include <QObject>
#include <QDebug>
//#include <QAudioOutput>

#include "modes.h"
#include "rbf.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
//#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/pixfmt.h>
#include <libavutil/imgutils.h>
}

class VideoDecoder : public QObject
{
Q_OBJECT

public:
    void endDecode();

private:
    AVFormatContext* format_ctx;
    AVCodecContext* codec_ctx_vid;
    AVFrame* frame_yuv;
    AVFrame* frame_rgb;
    SwsContext* sws_ctx;
    AVPacket* packet;
    int loop;

signals:
    void sigSendVideoFrame(AVFrame* frame, mode_video mode);

public slots:
    void slotDecodeLoop(mode_video mode);
};

class AudioDecoder : public QObject
{
Q_OBJECT
public:
    void endDecode();

private:
    AVFormatContext* format_ctx;
    AVCodecContext* codec_ctx_aud;
    AVFrame* frame;
    AVPacket* packet;
    int loop;
     
signals:
    void sigSendAudioFrame(AVFrame* frame, mode_audio mode);

public slots:
    void slotDecodeLoop(mode_audio mode);

};

