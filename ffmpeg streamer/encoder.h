#pragma once
#include <QObject>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/pixfmt.h>
#include <libavutil/imgutils.h>
}

class VideoEncoder : public QObject
{
	Q_OBJECT
public:
	VideoEncoder();
	~VideoEncoder();
private:
	AVCodecContext* codec_ctx;
	SwsContext* sws_ctx;
	//AVPacket* packet;
signals:
	void sigSendVideoPacket(AVPacket* packet);
public slots:
	void slotEncodeVideoFrameToPacket(AVFrame* frame);
};

class AudioEncoder : public QObject
{
	Q_OBJECT
public:
	AudioEncoder();
	~AudioEncoder();
private:
	AVCodecContext* codec_ctx;
	SwrContext* swr_ctx;
	uint8_t*  remaining_buffer[2];
	int remaining_size = 0;
	//AVFormatContext* output;
signals:
	void sigSendAudioPacket(AVPacket* packet);
public slots:
	void slotEncodeAudioFrameToPacket(AVFrame* frame);
};

