#include "encoder.h"

VideoEncoder::VideoEncoder()
{
	int ret = -1;

	// get encoder, alloc codec context
	const AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
	codec_ctx = avcodec_alloc_context3(encoder);

	codec_ctx->bit_rate = 5000000;
	codec_ctx->width = 1920;
	codec_ctx->height = 1080;
	codec_ctx->framerate = { 30,1 };
	codec_ctx->time_base = { 1,30 };
	codec_ctx->gop_size = 15;
	codec_ctx->max_b_frames = 0;
	codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
	av_opt_set(codec_ctx->priv_data, "preset", "slow", 0);
	av_opt_set(codec_ctx->priv_data, "tune", "zerolatency", 0);

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
			packet->stream_index = 0;
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
	codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
	codec_ctx->ch_layout = AV_CHANNEL_LAYOUT_STEREO;
	//codec_ctx->channel_layout = av_get_channel_layout("stereo");
	codec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
	codec_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	ret = avcodec_open2(codec_ctx, encoder, NULL);
	/*swr_ctx = swr_alloc_set_opts(NULL,
		AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLTP, 44100,
		AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100,
		0, NULL);*/
	remaining_size = 0;
	remaining_buffer[0] = NULL;
	remaining_buffer[1] = NULL;

	//ret = avformat_alloc_output_context2(&output, NULL,NULL, "O:\\_nginx_sample\\test.aac");
	//AVStream* audio_stream = avformat_new_stream(output, NULL);
	//avcodec_parameters_from_context(audio_stream->codecpar, codec_ctx);
	//ret = avio_open(&output->pb, "O:\\_nginx_sample\\test.aac", AVIO_FLAG_WRITE);
	//ret = avformat_write_header(output,NULL);
}

AudioEncoder::~AudioEncoder()
{
	avcodec_free_context(&codec_ctx);
	//swr_free(&swr_ctx);
	//av_write_trailer(output);
	//avformat_free_context(output);
	
}

void AudioEncoder::slotEncodeAudioFrameToPacket(AVFrame* frame)
{
	int ret = -1;
	/*AVFrame* frame_fltp = av_frame_alloc();
	ret = av_samples_alloc(frame_fltp->data, frame_fltp->linesize, 2, 44100/2, AV_SAMPLE_FMT_FLTP, 1);

	ret = swr_convert(swr_ctx,
		frame_fltp->data, frame_fltp->nb_samples,
		(const uint8_t**)frame->data, frame->nb_samples);*/
	int total_samples = frame->nb_samples;
	int samples_per_part = codec_ctx->frame_size;
	
	int bytes_per_sample = av_get_bytes_per_sample((AVSampleFormat)frame->format);

	uint8_t* temp_buf[2];
	temp_buf[0] = (uint8_t*)malloc(bytes_per_sample * (total_samples + remaining_size));
	temp_buf[1] = (uint8_t*)malloc(bytes_per_sample * (total_samples + remaining_size));
	memcpy(temp_buf[0], remaining_buffer[0], bytes_per_sample * remaining_size);
	memcpy(temp_buf[1], remaining_buffer[1], bytes_per_sample * remaining_size);
	memcpy(temp_buf[0] + bytes_per_sample * remaining_size, frame->data[0],
		bytes_per_sample * total_samples);
	memcpy(temp_buf[1] + bytes_per_sample * remaining_size, frame->data[1],
		bytes_per_sample * total_samples);
	remaining_size += total_samples;
	
	int num_parts = floor( (float)remaining_size / samples_per_part )+ 1;

	for (int i = 0; i < num_parts-1; i++)
	{
		int start_index = i * samples_per_part;
		//int num_samples = samples_per_part;//FFMIN(samples_per_part, total_samples - start_index);

		AVFrame* part_frame = av_frame_alloc();

		part_frame->format = frame->format;
		part_frame->sample_rate = frame->sample_rate;
		part_frame->channel_layout = frame->channel_layout;
		part_frame->channels = frame->channels;
		part_frame->nb_samples = samples_per_part;

		ret = av_frame_get_buffer(part_frame, 0);
		for (int j = 0; j < frame->channels; j++)
			memcpy(part_frame->data[j], temp_buf[j] + bytes_per_sample * start_index, samples_per_part * bytes_per_sample);
		
		AVPacket* packet = av_packet_alloc();
		if (avcodec_send_frame(codec_ctx, part_frame)==0)
		{
			while (avcodec_receive_packet(codec_ctx, packet)==0)
			{
				
				//ret = av_write_frame(output, packet);
				packet->stream_index = 1;
				emit sigSendAudioPacket(av_packet_clone(packet));
			}
		}
		av_packet_free(&packet);
		av_frame_free(&part_frame);
		remaining_size -= samples_per_part;
	}

	free(remaining_buffer[0]); free(remaining_buffer[1]);
	remaining_buffer[0] = (uint8_t*)malloc(bytes_per_sample * remaining_size);
	remaining_buffer[1] = (uint8_t*)malloc(bytes_per_sample * remaining_size);
	memcpy(remaining_buffer[0], temp_buf[0] + bytes_per_sample * ((num_parts - 1) * samples_per_part), bytes_per_sample * remaining_size);
	memcpy(remaining_buffer[1], temp_buf[1] + bytes_per_sample * ((num_parts - 1) * samples_per_part), bytes_per_sample * remaining_size);
	free(temp_buf[0]); free(temp_buf[1]);

	av_frame_free(&frame);
}