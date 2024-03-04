#include "encoder.h"

VideoEncoder::VideoEncoder()
{
	int ret = -1;

	// get encoder, alloc codec context
	const AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_HEVC);
	codec_ctx = avcodec_alloc_context3(encoder);

	codec_ctx->bit_rate = 5000000;
	codec_ctx->width = 1920;
	codec_ctx->height = 1080;
	codec_ctx->framerate = { 30,1 };
	codec_ctx->time_base = { 1,30 };
	codec_ctx->gop_size = 30;
	codec_ctx->max_b_frames = 3;
	codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;

	ret = avcodec_open2(codec_ctx, encoder, NULL);

	sws_ctx = sws_getContext(1920, 1080, AV_PIX_FMT_RGB24,
		1920, 1080, AV_PIX_FMT_YUV420P,
		SWS_BILINEAR, NULL, NULL, NULL);
}

VideoEncoder::~VideoEncoder()
{
	avcodec_free_context(&codec_ctx);
	sws_freeContext(sws_ctx);
}

void VideoEncoder::slotEncodeVideoFrameToPacket(AVFrame* frame)
{
	int ret = -1;

	AVFrame* frame_yuv = av_frame_alloc();
	ret = av_image_alloc(frame_yuv->data, frame_yuv->linesize, 1920, 1080, AV_PIX_FMT_YUV420P, 1);

	ret = sws_scale(sws_ctx,
		frame->data,
		frame->linesize,
		0,
		codec_ctx->height, frame_yuv->data,
		frame_yuv->linesize);

	frame_yuv->format = AV_PIX_FMT_YUV420P;
	frame_yuv->width = frame->width;
	frame_yuv->height = frame->height;


	AVPacket* packet = av_packet_alloc();
	if (!avcodec_send_frame(codec_ctx, frame_yuv))
	{
		while (!avcodec_receive_packet(codec_ctx, packet))
		{
			emit sigSendVideoPacket(av_packet_clone(packet));
		}
	}
	av_packet_free(&packet);
	av_freep(&frame_yuv->data[0]);
	av_frame_free(&frame_yuv);
	av_frame_free(&frame);
}

AudioEncoder::AudioEncoder()
{
	int ret = -1;

	// get encoder, alloc codec context
	const AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
	codec_ctx = avcodec_alloc_context3(encoder);

	codec_ctx->bit_rate = 128000;
	codec_ctx->sample_rate = 44100;
	codec_ctx->time_base = { 1, 44100 };
	codec_ctx->channels = 2;
	codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
	codec_ctx->ch_layout = AV_CHANNEL_LAYOUT_STEREO;
	codec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
	//codec_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	ret = avcodec_open2(codec_ctx, encoder, NULL);
}

AudioEncoder::~AudioEncoder()
{
	avcodec_free_context(&codec_ctx);
}

void AudioEncoder::slotEncodeAudioFrameToPacket(AVFrame* frame)
{
	AVPacket* packet = av_packet_alloc();
	if (!avcodec_send_frame(codec_ctx, frame))
	{
		while (!avcodec_receive_packet(codec_ctx, packet))
		{
			emit sigSendAudioPacket(av_packet_clone(packet));
		}
	}
	av_packet_free(&packet);
	av_frame_free(&frame);
}