#include "muxer.h"

Muxer::Muxer(AVCodecContext* ctx_video, AVCodecContext* ctx_audio)
{
	int ret = -1;

	//avformat_network_init();
	codec_context_video = ctx_video;
	codec_context_audio = ctx_audio;

	ret = avformat_alloc_output_context2(&output, NULL, "flv",
		"O://_nginx_sample/test.flv"/*"rtmp://localhost:1935/live/damn"*/
		/*"rtmp://live-push.bilivideo.com/live-bvc/?streamname=live_36154769_2980266&key=bbc9a90fcdbe95cfa2754cebd68b409d&schedule=rtmp&pflag=1"*/);
	/*AVStream* video_stream = avformat_new_stream(output, NULL);
	avcodec_parameters_from_context(video_stream->codecpar, codec_context_video);
	AVStream* audio_stream = avformat_new_stream(output, NULL);
	avcodec_parameters_from_context(audio_stream->codecpar, codec_context_audio);*/
	AVStream* video_stream = avformat_new_stream(output, codec_context_video->codec);
	avcodec_parameters_from_context(video_stream->codecpar, codec_context_video);
	AVStream* audio_stream = avformat_new_stream(output, codec_context_audio->codec);
	avcodec_parameters_from_context(audio_stream->codecpar, codec_context_audio);

	video_stream->time_base = {1,90000};
	//audio_stream->time_base = codec_context_audio->time_base;

	ret = avio_open(&output->pb, "O://_nginx_sample/test.flv", AVIO_FLAG_WRITE);
	ret = avformat_write_header(output,NULL);

	start_time = av_gettime();
}

Muxer::~Muxer()
{
	av_write_trailer(output);
	avformat_free_context(output);
}

void Muxer::slotWritePacket(AVPacket* packet)
{
	// I really wonder how this works...
	if (packet->stream_index == 0 /*&& packet->pts == AV_NOPTS_VALUE*/) {

		//Write PTS
		AVRational time_base = {1,1000};
		//Duration between 2 frames (us)
		int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(codec_context_video->framerate);
		//Parameters
		packet->pts = (double)(frame_index++ * calc_duration) / (double)(av_q2d(time_base) * AV_TIME_BASE);
		packet->dts = packet->pts;
		packet->duration = (double)calc_duration / (double)(av_q2d(time_base) * AV_TIME_BASE);

		/*AVRational time_base_q = {1,AV_TIME_BASE};
		if (start_time == -1)
			start_time = av_gettime();
		int64_t pts_time = av_rescale_q(packet->dts, time_base, time_base_q);
		int64_t now_time = av_gettime() - start_time;
		if (pts_time > now_time)
			av_usleep(pts_time - now_time);*/
	}
	
	av_write_frame(output, packet);
	av_packet_free(&packet);
}