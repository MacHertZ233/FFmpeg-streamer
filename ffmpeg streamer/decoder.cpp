#include "decoder.h"

void VideoDecoder::decode(mode_video mode)
{
	avdevice_register_all();

	const AVInputFormat* ifmt;
	if (mode == MODE_VID_CAM)
		ifmt = av_find_input_format("dshow");
	else if (mode == MODE_VID_SCR)
		ifmt = av_find_input_format("gdigrab");


	int ret = -1;

	// get format context
	format_ctx = avformat_alloc_context();
	if (mode == MODE_VID_CAM)
		ret = avformat_open_input(&format_ctx, "video=Integrated Camera", ifmt, NULL);
	else if (mode == MODE_VID_SCR)
	{
		AVDictionary* option = NULL;
		av_dict_set(&option, "framerate", "60", 0);
		ret = avformat_open_input(&format_ctx, "desktop", ifmt, &option);
	}
	ret = avformat_find_stream_info(format_ctx, NULL);

	// get stream index & codec
	const AVCodec* codec_vid, * codec_aud;
	int videoindex = av_find_best_stream(format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec_vid, 0);
	//int audioindex = av_find_best_stream(format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &codec_aud, 0);

	// get context, open video codec
	codec_ctx_vid = avcodec_alloc_context3(codec_vid);
	avcodec_parameters_to_context(codec_ctx_vid, format_ctx->streams[videoindex]->codecpar);
	ret = avcodec_open2(codec_ctx_vid, codec_vid, NULL);


	// init frame container
	frame_yuv = av_frame_alloc();
	frame_rgb = av_frame_alloc();
	ret = av_image_alloc(frame_rgb->data, frame_rgb->linesize, codec_ctx_vid->width, codec_ctx_vid->height, AV_PIX_FMT_RGB24, 1);

	// init sws content
	sws_ctx = sws_getContext(codec_ctx_vid->width, codec_ctx_vid->height, codec_ctx_vid->pix_fmt,
		codec_ctx_vid->width, codec_ctx_vid->height, AV_PIX_FMT_RGB24,
		SWS_BILINEAR, NULL, NULL, NULL);

	// alloc space for packet container
	packet = (AVPacket*)av_malloc(sizeof(AVPacket));
	ret = av_new_packet(packet, codec_ctx_vid->width * codec_ctx_vid->height);

	//FILE* f = fopen("O:\\_nginx_sample\\cam.rgb", "wb");
	while (1)
	{
		if (av_read_frame(format_ctx, packet) < 0) break;

		if (packet->stream_index == videoindex)
		{

			ret = avcodec_send_packet(codec_ctx_vid, packet);
			if (ret == AVERROR_INVALIDDATA) continue;
			if (ret == 0)
			{
				while (avcodec_receive_frame(codec_ctx_vid, frame_yuv) == 0)
				{
					/*qDebug() << "received frame"
						<< " format " << QString::number(avframe->format)
						<< " width " << QString::number(avframe->width)
						<< " height " << QString::number(avframe->height)
						<< " duration " << QString::number(avframe->duration)
						<< " is_key_frame " << QString::number(avframe->key_frame);*/

					int fff = sws_scale(sws_ctx,
						(const uint8_t* const*)frame_yuv->data,
						frame_yuv->linesize,
						0,
						codec_ctx_vid->height,
						frame_rgb->data,
						frame_rgb->linesize);

					/*qDebug() << "converted frame"
						<< " format " << QString::number(avframergb->format)
						<< " width " << QString::number(avframergb->width)
						<< " height " << QString::number(avframergb->height)
						<< " duration " << QString::number(avframergb->duration)
						<< " is_key_frame " << QString::number(avframergb->key_frame)
						<< " return " << fff;*/

					frame_rgb->format = AV_PIX_FMT_RGB24;
					frame_rgb->width = frame_yuv->width;
					frame_rgb->height = frame_yuv->height;
					//avframergb->pts = avframe->pts;
					//avframergb->pkt_dts = avframe->pkt_dts;

					emit sigSendVideoFrame(frame_rgb, mode);

					//fwrite(avframergb->data[0], 1, avframergb->linesize[0] * codec_ctx_vid->height, f);
				}

			}

		}
		av_packet_unref(packet);
	}
	//fclose(f);


	av_free(frame_yuv);
	av_free(frame_rgb);
	sws_freeContext(sws_ctx);
	avcodec_close(codec_ctx_vid);
	avformat_close_input(&format_ctx);
}

void AudioDecoder::decode(mode_audio mode)
{
	avdevice_register_all();
	const AVInputFormat* ifmt = av_find_input_format("dshow");

	int ret = -1;

	// get format context
	format_ctx = avformat_alloc_context();
	if (mode == MODE_AUD_MIC)
		ret = avformat_open_input(&format_ctx, "audio=麦克风阵列 (Realtek(R) Audio)", ifmt, NULL);
	else if (mode == MODE_AUD_SYS)
		ret = avformat_open_input(&format_ctx, "audio=virtual-audio-capturer", ifmt, NULL);

	ret = avformat_find_stream_info(format_ctx, NULL);

	// get stream index & codec
	const AVCodec* codec_aud;
	//int videoindex = av_find_best_stream(format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec_vid, 0);
	int audioindex = av_find_best_stream(format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &codec_aud, 0);

	// get context, open audio codec
	codec_ctx_aud = avcodec_alloc_context3(codec_aud);
	avcodec_parameters_to_context(codec_ctx_aud, format_ctx->streams[audioindex]->codecpar);
	ret = avcodec_open2(codec_ctx_aud, codec_aud, NULL);


	// init frame container
	frame = av_frame_alloc();
	//avframes16 = av_frame_alloc();


	// init sws content
	/*SwrContext* swr_ctx = swr_alloc_set_opts(nullptr,
		codec_ctx_aud->channel_layout, AV_SAMPLE_FMT_S16, codec_ctx_aud->sample_rate,
		codec_ctx_aud->channel_layout, codec_ctx_aud->sample_fmt, codec_ctx_aud->sample_rate,
		0, nullptr);
	swr_init(swr_ctx);*/

	// alloc space for packet container
	packet = (AVPacket*)av_malloc(sizeof(AVPacket));
	ret = av_new_packet(packet, codec_ctx_aud->width * codec_ctx_aud->height);

	//FILE* f = fopen("O:\\_nginx_sample\\mic.pcm", "wb");
	while (1)
	{
		if (av_read_frame(format_ctx, packet) < 0) break;

		if (packet->stream_index == audioindex)
		{

			ret = avcodec_send_packet(codec_ctx_aud, packet);
			if (ret == AVERROR_INVALIDDATA) continue;
			if (ret == 0)
			{
				while (avcodec_receive_frame(codec_ctx_aud, frame) == 0)
				{
					/*qDebug() << "received frame"
						<< " format " << QString::number(frame->format)
						<< " sample_rate " << QString::number(frame->sample_rate)
						<< " nb_samples " << QString::number(frame->nb_samples)
						<< " channels " << QString::number(frame->channels)
						<< " duration " << QString::number(frame->duration);*/


					//int dstNbSamples = av_rescale_rnd(swr_get_delay(swr_ctx, avframe->sample_rate) + avframe->nb_samples,
					//    44100, avframe->sample_rate, AV_ROUND_UP);
					//int dstBufferSize = av_samples_get_buffer_size(NULL, 2, dstNbSamples, AV_SAMPLE_FMT_S16, 0);


					//ret = swr_convert(swr_ctx,
					//    avframes16->data, dstNbSamples,
					//    (const uint8_t**)avframe->data, avframe->nb_samples);


					//qDebug() << "converted frame"
					//    << " format " << QString::number(avframes16->format)
					//    << " sample_rate " << QString::number(avframes16->sample_rate)
					//    << " nb_samples " << QString::number(avframes16->nb_samples)
					//    << " channels " << QString::number(avframes16->channels)
					//    << " duration " << QString::number(avframes16->duration);

					emit sigSendAudioFrame(av_frame_clone(frame), mode);

					/*int frameBytes = avframe->nb_samples
						* avframe->channels
						* av_get_bytes_per_sample((AVSampleFormat)avframe->format);
					fwrite(avframe->data[0], 1, frameBytes, f);*/
				}

			}

		}
		av_packet_unref(packet);
	}
	//fclose(f);


	av_free(frame);
	//av_free(avframes16);
	//swr_free(&swr_ctx);
	avcodec_close(codec_ctx_aud);
	avformat_close_input(&format_ctx);




}