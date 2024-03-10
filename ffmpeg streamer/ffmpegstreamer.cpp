#include "ffmpegstreamer.h"



FFmpegStreamer::FFmpegStreamer(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	videoLabel = ui.label;

	// video merger thread
	QThread* mergeVideoThread = new QThread;
	merger_video = new VideoMerger;
	merger_video->moveToThread(mergeVideoThread);
	mergeVideoThread->start();

	// cam decoder thread
	QThread* camThread = new QThread;
	VideoDecoder* camDecoder = new VideoDecoder;
	camDecoder->moveToThread(camThread);
	camThread->start();

	// screen capture decoder thread
	QThread* scrThread = new QThread;
	VideoDecoder* scrDecoder = new VideoDecoder;
	scrDecoder->moveToThread(scrThread);
	scrThread->start();

	// audio merger thread
	QThread* mergeAudioThread = new QThread;
	merger_audio = new AudioMerger;
	merger_audio->moveToThread(mergeAudioThread);
	mergeAudioThread->start();

	// mic decoder thread
	QThread* micThread = new QThread;
	AudioDecoder* micDecoder = new AudioDecoder;
	micDecoder->moveToThread(micThread);
	micThread->start();

	// system sound decoder thread
	QThread* sysThread = new QThread;
	AudioDecoder* sysDecoder = new AudioDecoder;
	sysDecoder->moveToThread(sysThread);
	sysThread->start();

	// video encoder thread
	QThread* encVidThread = new QThread;
	VideoEncoder* vidEncoder = new VideoEncoder;
	vidEncoder->moveToThread(encVidThread);
	encVidThread->start();

	// audio encoder thread
	QThread* encAudThread = new QThread;
	AudioEncoder* audEncoder = new AudioEncoder;
	audEncoder->moveToThread(encAudThread);
	encAudThread->start();

	// muxer & streamer thread
	QThread* muxThread = new QThread;
	Muxer* muxer = new Muxer(
		vidEncoder->getCodecContext(),
		audEncoder->getCodecContext()
	);
	muxer->moveToThread(muxThread);
	muxThread->start();

	connect(this, &FFmpegStreamer::sigDecodeVideoCam, camDecoder, &VideoDecoder::decode);
	connect(this, &FFmpegStreamer::sigDecodeVideoScr, scrDecoder, &VideoDecoder::decode);
	connect(this, &FFmpegStreamer::sigMergeVideoLoop, merger_video, &VideoMerger::slotMergeVideoLoop);
	connect(camDecoder, &VideoDecoder::sigSendVideoFrame, this, &FFmpegStreamer::slotReceiveVideoFrame);
	connect(scrDecoder, &VideoDecoder::sigSendVideoFrame, this, &FFmpegStreamer::slotReceiveVideoFrame);
	connect(merger_video, &VideoMerger::sigSendMergedVideoFrame, this, &FFmpegStreamer::slotReceiveMergedVideoFrame);

	connect(this, &FFmpegStreamer::sigDecodeAudioMic, micDecoder, &AudioDecoder::decode);
	connect(this, &FFmpegStreamer::sigDecodeAudioSys, sysDecoder, &AudioDecoder::decode);
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

	//f = fopen("O:\\_nginx_sample\\test.aac", "wb");



}

FFmpegStreamer::~FFmpegStreamer()
{
	//fclose(f);
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
		videoLabel->setFixedSize(img.size()/2);
		QPixmap map = QPixmap::fromImage(img);
		videoLabel->setPixmap(map.scaled(videoLabel->size()));

		emit sigEncodeVideoFrame(frame);

		
	}

}

void FFmpegStreamer::slotReceiveVideoPacket(AVPacket* packet)
{
	//fwrite(packet->data, 1, packet->size, f);
	emit sigOutputPacket(packet);
	//av_packet_free(&packet);
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
	//av_packet_free(&packet);
}
