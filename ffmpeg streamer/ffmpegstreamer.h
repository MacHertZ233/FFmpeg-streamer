#pragma once

#include <QtWidgets/QMainWindow>
#include <QAudioFormat>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QThread>
#include <QDebug>
#include <QImage>
#include <QPainter>
#include "ui_ffmpegstreamer.h"
#include "decoder.h"
#include "merger.h"
#include "encoder.h"
#include "muxer.h"
#include "rbf.hpp"

#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
#include <libavutil/imgutils.h>
//#include <libavutil/log.h>
}

using namespace std;

class FFmpegStreamer : public QMainWindow
{
    Q_OBJECT

public:
    FFmpegStreamer(QWidget *parent = nullptr);
    ~FFmpegStreamer();

private:
    // UI及其组件管理
    Ui::ffmpegstreamerClass ui;
    QLabel* label_video;
    QLineEdit* line_edit_address;
    QComboBox* combo_box_position;
    QSpinBox* spin_box_width, * spin_box_height;
    QDoubleSpinBox* spin_box_weight;
    QPushButton* button_start;

    // 线程管理
    QThread* camThread, * scrThread, * mergeVideoThread,
        * micThread, * sysThread, * mergeAudioThread,
        * encVidThread, * encAudThread, * muxThread;
    
    // 对象管理
    VideoDecoder* camDecoder, * scrDecoder;
    AudioDecoder* micDecoder, * sysDecoder;
    VideoMerger* merger_video;
    AudioMerger* merger_audio;
    VideoEncoder* vidEncoder;
    AudioEncoder* audEncoder;
    Muxer* muxer;

signals:
    void sigMergeVideoLoop();
    void sigDecodeVideoCam(mode_video mode);
    void sigDecodeVideoScr(mode_video mode);
    void sigMergeAudioLoop();
    void sigDecodeAudioMic(mode_audio mode);
    void sigDecodeAudioSys(mode_audio mode);
    void sigEncodeVideoFrame(AVFrame* frame);
    void sigEncodeAudioFrame(AVFrame* frame);
    void sigOutputPacket(AVPacket* packet);

private slots:
    void slotStartStreaming();
    void slotStopStreaming();
    void slotReceiveVideoFrame(AVFrame* frame, mode_video mode);
    void slotReceiveMergedVideoFrame(AVFrame* frame);
    void slotReceiveAudioFrame(AVFrame* frame, mode_audio mode);
    void slotReceiveMergedAudioFrame(AVFrame* frame);
    void slotReceiveVideoPacket(AVPacket* packet);
    void slotReceiveAudioPacket(AVPacket* packet);
};
