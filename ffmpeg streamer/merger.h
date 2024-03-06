#pragma once

#include <QObject>
#include <QQueue>
#include <QMutex>
#include "modes.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
//#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
//#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/pixfmt.h>
//#include <libavutil/imgutils.h>
}

class VideoMerger : public QObject
{
	Q_OBJECT

public:
	VideoMerger();
	QQueue<AVFrame*> queue_vid_cam;
	QQueue<AVFrame*> queue_vid_scr;
	QMutex mutex;

private:
	AVFilterGraph* filter_graph;
	AVFilterContext* src_cam, * src_scr, * overlay, * fps, * sink;

signals:
	void sigSendMergedVideoFrame(AVFrame* frame);

public slots:
	void slotMergeVideoLoop();
};

class AudioMerger :public QObject
{
	Q_OBJECT

public:
	AudioMerger();
	QQueue<AVFrame*> queue_aud_mic;
	QQueue<AVFrame*> queue_aud_sys;
	QMutex mutex;

private:
	AVFilterGraph* filter_graph;
	AVFilterContext* src_mic, * src_sys, * amerge, * pan, * aformat, * sink;

signals:
	void sigSendMergedAudioFrame(AVFrame* frame);

public slots:
	void slotMergeAudioLoop();
};

