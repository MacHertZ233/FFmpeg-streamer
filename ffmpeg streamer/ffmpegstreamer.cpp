#include "ffmpegstreamer.h"

FFmpegStreamer::FFmpegStreamer(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    label_video = ui.label;

    line_edit_address = ui.lineEditAddress;
    combo_box_position = ui.comboBoxLocation;
    spin_box_width = ui.spinBoxWidth;
    spin_box_height = ui.spinBoxHeight;
    spin_box_weight = ui.doubleSpinBoxWeight;
    button_start = ui.pushButtonStart;

    connect(button_start, &QPushButton::pressed, this, &FFmpegStreamer::slotStartStreaming);
}

FFmpegStreamer::~FFmpegStreamer()
{
    //fclose(f);
}

void FFmpegStreamer::slotStartStreaming()
{

    button_start->setText("停止推流");
    disconnect(button_start, &QPushButton::pressed, this, &FFmpegStreamer::slotStartStreaming);
    connect(button_start, &QPushButton::pressed, this, &FFmpegStreamer::slotStopStreaming);

    line_edit_address->setDisabled(1);
    combo_box_position->setDisabled(1);
    spin_box_width->setDisabled(1);
    spin_box_height->setDisabled(1);
    spin_box_weight->setDisabled(1);

    // cam decoder thread
    camThread = new QThread;
    camDecoder = new VideoDecoder;
    camDecoder->moveToThread(camThread);
    camThread->start();
    connect(camThread, &QThread::finished, camDecoder, &VideoDecoder::deleteLater);

    // screen capture decoder thread
    scrThread = new QThread;
    scrDecoder = new VideoDecoder;
    scrDecoder->moveToThread(scrThread);
    scrThread->start();
    connect(scrThread, &QThread::finished, scrDecoder, &VideoDecoder::deleteLater);

    // video merger thread
    mergeVideoThread = new QThread;
    merger_video = new VideoMerger(spin_box_width->value(), spin_box_height->value(), combo_box_position->currentIndex());
    merger_video->moveToThread(mergeVideoThread);
    mergeVideoThread->start();
    connect(mergeVideoThread, &QThread::finished, merger_video, &VideoMerger::deleteLater);

    // mic decoder thread
    micThread = new QThread;
    micDecoder = new AudioDecoder;
    micDecoder->moveToThread(micThread);
    micThread->start();
    connect(micThread, &QThread::finished, micDecoder, &AudioDecoder::deleteLater);

    // system sound decoder thread
    sysThread = new QThread;
    sysDecoder = new AudioDecoder;
    sysDecoder->moveToThread(sysThread);
    sysThread->start();
    connect(sysThread, &QThread::finished, sysDecoder, &AudioDecoder::deleteLater);

    // audio merger thread
    mergeAudioThread = new QThread;
    merger_audio = new AudioMerger(spin_box_weight->value());
    merger_audio->moveToThread(mergeAudioThread);
    mergeAudioThread->start();
    connect(mergeAudioThread, &QThread::finished, merger_audio, &AudioMerger::deleteLater);

    // video encoder thread
    encVidThread = new QThread;
    vidEncoder = new VideoEncoder;
    vidEncoder->moveToThread(encVidThread);
    encVidThread->start();
    connect(encVidThread, &QThread::finished, vidEncoder, &VideoEncoder::deleteLater);

    // audio encoder thread
    encAudThread = new QThread;
    audEncoder = new AudioEncoder;
    audEncoder->moveToThread(encAudThread);
    encAudThread->start();
    connect(encAudThread, &QThread::finished, audEncoder, &AudioEncoder::deleteLater);

    // muxer & streamer thread
    muxThread = new QThread;
    muxer = new Muxer(
        vidEncoder->getCodecContext(),
        audEncoder->getCodecContext(),
        line_edit_address->text().toStdString().data()
    );
    muxer->moveToThread(muxThread);
    muxThread->start();
    connect(muxThread, &QThread::finished, muxer, &Muxer::deleteLater);

    connect(this, &FFmpegStreamer::sigDecodeVideoCam, camDecoder, &VideoDecoder::slotDecodeLoop);
    connect(this, &FFmpegStreamer::sigDecodeVideoScr, scrDecoder, &VideoDecoder::slotDecodeLoop);
    connect(this, &FFmpegStreamer::sigMergeVideoLoop, merger_video, &VideoMerger::slotMergeVideoLoop);
    connect(camDecoder, &VideoDecoder::sigSendVideoFrame, this, &FFmpegStreamer::slotReceiveVideoFrame);
    connect(scrDecoder, &VideoDecoder::sigSendVideoFrame, this, &FFmpegStreamer::slotReceiveVideoFrame);
    connect(merger_video, &VideoMerger::sigSendMergedVideoFrame, this, &FFmpegStreamer::slotReceiveMergedVideoFrame);

    connect(this, &FFmpegStreamer::sigDecodeAudioMic, micDecoder, &AudioDecoder::slotDecodeLoop);
    connect(this, &FFmpegStreamer::sigDecodeAudioSys, sysDecoder, &AudioDecoder::slotDecodeLoop);
    connect(this, &FFmpegStreamer::sigMergeAudioLoop, merger_audio, &AudioMerger::slotMergeAudioLoop);
    connect(micDecoder, &AudioDecoder::sigSendAudioFrame, this, &FFmpegStreamer::slotReceiveAudioFrame);
    connect(sysDecoder, &AudioDecoder::sigSendAudioFrame, this, &FFmpegStreamer::slotReceiveAudioFrame);
    connect(merger_audio, &AudioMerger::sigSendMergedAudioFrame, this, &FFmpegStreamer::slotReceiveMergedAudioFrame);

    connect(this, &FFmpegStreamer::sigEncodeVideoFrame, vidEncoder, &VideoEncoder::slotEncodeVideoFrameToPacket);
    connect(vidEncoder, &VideoEncoder::sigSendVideoPacket, this, &FFmpegStreamer::slotReceiveVideoPacket);

    connect(this, &FFmpegStreamer::sigEncodeAudioFrame, audEncoder, &AudioEncoder::slotEncodeAudioFrameToPacket);
    connect(audEncoder, &AudioEncoder::sigSendAudioPacket, this, &FFmpegStreamer::slotReceiveAudioPacket);

    connect(this, &FFmpegStreamer::sigOutputPacket, muxer, &Muxer::slotWritePacket);

    emit sigMergeVideoLoop();
    emit sigDecodeVideoScr(MODE_VID_SCR);
    emit sigDecodeVideoCam(MODE_VID_CAM);

    emit sigMergeAudioLoop();
    emit sigDecodeAudioMic(MODE_AUD_MIC);
    emit sigDecodeAudioSys(MODE_AUD_SYS);

}

void FFmpegStreamer::slotStopStreaming()
{
    button_start->setText("开始推流");
    connect(button_start, &QPushButton::pressed, this, &FFmpegStreamer::slotStartStreaming);
    disconnect(button_start, &QPushButton::pressed, this, &FFmpegStreamer::slotStopStreaming);

    line_edit_address->setDisabled(0);
    combo_box_position->setDisabled(0);
    spin_box_width->setDisabled(0);
    spin_box_height->setDisabled(0);
    spin_box_weight->setDisabled(0);
    label_video->setPixmap(QPixmap(""));

    camDecoder->endDecode();
    scrDecoder->endDecode();
    micDecoder->endDecode();
    sysDecoder->endDecode();
    merger_video->endMerge();
    merger_audio->endMerge();

    disconnect(this, &FFmpegStreamer::sigDecodeVideoCam, camDecoder, &VideoDecoder::slotDecodeLoop);
    disconnect(this, &FFmpegStreamer::sigDecodeVideoScr, scrDecoder, &VideoDecoder::slotDecodeLoop);
    disconnect(this, &FFmpegStreamer::sigMergeVideoLoop, merger_video, &VideoMerger::slotMergeVideoLoop);
    disconnect(camDecoder, &VideoDecoder::sigSendVideoFrame, this, &FFmpegStreamer::slotReceiveVideoFrame);
    disconnect(scrDecoder, &VideoDecoder::sigSendVideoFrame, this, &FFmpegStreamer::slotReceiveVideoFrame);
    disconnect(merger_video, &VideoMerger::sigSendMergedVideoFrame, this, &FFmpegStreamer::slotReceiveMergedVideoFrame);

    disconnect(this, &FFmpegStreamer::sigDecodeAudioMic, micDecoder, &AudioDecoder::slotDecodeLoop);
    disconnect(this, &FFmpegStreamer::sigDecodeAudioSys, sysDecoder, &AudioDecoder::slotDecodeLoop);
    disconnect(this, &FFmpegStreamer::sigMergeAudioLoop, merger_audio, &AudioMerger::slotMergeAudioLoop);
    disconnect(micDecoder, &AudioDecoder::sigSendAudioFrame, this, &FFmpegStreamer::slotReceiveAudioFrame);
    disconnect(sysDecoder, &AudioDecoder::sigSendAudioFrame, this, &FFmpegStreamer::slotReceiveAudioFrame);
    disconnect(merger_audio, &AudioMerger::sigSendMergedAudioFrame, this, &FFmpegStreamer::slotReceiveMergedAudioFrame);

    disconnect(this, &FFmpegStreamer::sigEncodeVideoFrame, vidEncoder, &VideoEncoder::slotEncodeVideoFrameToPacket);
    disconnect(vidEncoder, &VideoEncoder::sigSendVideoPacket, this, &FFmpegStreamer::slotReceiveVideoPacket);

    disconnect(this, &FFmpegStreamer::sigEncodeAudioFrame, audEncoder, &AudioEncoder::slotEncodeAudioFrameToPacket);
    disconnect(audEncoder, &AudioEncoder::sigSendAudioPacket, this, &FFmpegStreamer::slotReceiveAudioPacket);

    disconnect(this, &FFmpegStreamer::sigOutputPacket, muxer, &Muxer::slotWritePacket);

    QThread::msleep(1000);

    camThread->quit(); scrThread->quit(); micThread->quit(); sysThread->quit();
    mergeVideoThread->quit(); mergeAudioThread->quit();
    encVidThread->quit(); encAudThread->quit();
    muxThread->quit();

    camThread->wait(); scrThread->wait(); micThread->wait(); sysThread->wait();
    mergeVideoThread->wait(); mergeAudioThread->wait();
    encVidThread->wait(); encAudThread->wait();
    muxThread->wait();

    disconnect(camThread, &QThread::finished, camDecoder, &VideoDecoder::deleteLater);
    disconnect(scrThread, &QThread::finished, scrDecoder, &VideoDecoder::deleteLater);
    disconnect(mergeVideoThread, &QThread::finished, merger_video, &VideoMerger::deleteLater);
    disconnect(micThread, &QThread::finished, micDecoder, &AudioDecoder::deleteLater);
    disconnect(sysThread, &QThread::finished, sysDecoder, &AudioDecoder::deleteLater);
    disconnect(mergeAudioThread, &QThread::finished, merger_audio, &AudioMerger::deleteLater);
    disconnect(encVidThread, &QThread::finished, vidEncoder, &VideoEncoder::deleteLater);
    disconnect(encAudThread, &QThread::finished, audEncoder, &AudioEncoder::deleteLater);
    disconnect(muxThread, &QThread::finished, muxer, &Muxer::deleteLater);
}

void FFmpegStreamer::slotReceiveVideoFrame(AVFrame* frame, mode_video mode)
{
    // enqueue video frame
    merger_video->mutex.lock();
    if (mode == MODE_VID_CAM)
        merger_video->queue_vid_cam.enqueue(frame);
    else if (mode == MODE_VID_SCR)
        merger_video->queue_vid_scr.enqueue(frame);
    merger_video->mutex.unlock();
}

void FFmpegStreamer::slotReceiveMergedVideoFrame(AVFrame* frame)
{
    if (frame)
    {
        // video loopback
        QImage img((uchar*)frame->data[0], frame->width, frame->height, QImage::Format_RGB888);
        label_video->setFixedSize(img.size() / 2);
        QPixmap map = QPixmap::fromImage(img);
        label_video->setPixmap(map.scaled(label_video->size()));

        emit sigEncodeVideoFrame(frame);
    }

}

void FFmpegStreamer::slotReceiveVideoPacket(AVPacket* packet)
{
    //fwrite(packet->data, 1, packet->size, f);
    emit sigOutputPacket(packet);
}

void FFmpegStreamer::slotReceiveAudioFrame(AVFrame* frame, mode_audio mode)
{
    // enqueue audio frame
    merger_audio->mutex.lock();
    if (mode == MODE_AUD_MIC)
        merger_audio->queue_aud_mic.enqueue(frame);
    else if (mode == MODE_AUD_SYS)
        merger_audio->queue_aud_sys.enqueue(frame);
    merger_audio->mutex.unlock();
}

void FFmpegStreamer::slotReceiveMergedAudioFrame(AVFrame* frame)
{

    if (frame)
    {
        // audio loopback?


        /*qDebug() << "merged frame"
            << " format " << QString::number(frame->format)
            << " sample_rate " << QString::number(frame->sample_rate)
            << " nb_samples " << QString::number(frame->nb_samples)
            << " channels " << QString::number(frame->channels)
            << " duration " << QString::number(frame->duration);*/

            /*int frameBytes = frame->nb_samples
                * frame->channels
                * av_get_bytes_per_sample((AVSampleFormat)frame->format);
            fwrite(frame->data[0], 1, frameBytes, f);*/

        emit sigEncodeAudioFrame(frame);

        //av_frame_free(&frame);
    }
}

void FFmpegStreamer::slotReceiveAudioPacket(AVPacket* packet)
{
    emit sigOutputPacket(packet);
}
