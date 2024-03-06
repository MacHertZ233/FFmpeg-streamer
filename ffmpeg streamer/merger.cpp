#include "merger.h"

VideoMerger::VideoMerger()
{
	// init filter graph

	int ret = -1;

	filter_graph = avfilter_graph_alloc();

	ret = avfilter_graph_create_filter(&src_cam, avfilter_get_by_name("buffer"), "src_cam",
		"video_size=640x360:pix_fmt=2:time_base=1/30:frame_rate=30:pixel_aspect=1/1", NULL, filter_graph);
	ret = avfilter_graph_create_filter(&src_scr, avfilter_get_by_name("buffer"), "src_scr",
		"video_size=1920x1080:pix_fmt=2:time_base=1/30:frame_rate=30:pixel_aspect=1/1", NULL, filter_graph);
	ret = avfilter_graph_create_filter(&overlay, avfilter_get_by_name("overlay"), "overlay",
		"x=0:y=0:format=rgb", NULL, filter_graph);
	ret = avfilter_graph_create_filter(&sink, avfilter_get_by_name("buffersink"), "sink",
		NULL, NULL, filter_graph);

	ret = avfilter_link(src_scr, 0, overlay, 0);
	ret = avfilter_link(src_cam, 0, overlay, 1);
	ret = avfilter_link(overlay, 0, sink, 0);

	//avfilter_link(src_scr, 0, sink, 0);

	ret = avfilter_graph_config(filter_graph, NULL);

}

void VideoMerger::slotMergeVideoLoop()
{
	int ret = -1;
	while (1)
	{
		if (!(queue_vid_cam.isEmpty() || queue_vid_scr.isEmpty()))
		{
			AVFrame* frame_cam, * frame_scr;

			mutex.lock();
			frame_cam = queue_vid_cam.dequeue();
			frame_scr = queue_vid_scr.dequeue();
			mutex.unlock();

			ret = av_buffersrc_add_frame(src_cam, frame_cam);
			ret = av_buffersrc_add_frame(src_scr, frame_scr);
			
			AVFrame* frame_merge = av_frame_alloc();
			ret = av_buffersink_get_frame(sink, frame_merge);
			emit sigSendMergedVideoFrame(frame_merge);
		}

		/*if (!queue_vid_cam.isEmpty())
		{
			AVFrame* frame_cam;

			mutex.lock();
			frame_cam = queue_vid_cam.dequeue();
			
			mutex.unlock();

			ret = av_buffersrc_add_frame(src_cam, frame_cam);

		}
		if (!queue_vid_scr.isEmpty())
		{
			AVFrame* frame_scr;
			
			mutex.lock();
			frame_scr = queue_vid_scr.dequeue();
			mutex.unlock();
			
			ret = av_buffersrc_add_frame(src_scr, frame_scr);
		}


		AVFrame* frame_merge = av_frame_alloc();
		
		if (av_buffersink_get_frame(sink, frame_merge) >= 0)
			emit sigSendMergedVideoFrame(frame_merge);
		else av_frame_free(&frame_merge);*/
	}
}


AudioMerger::AudioMerger()
{
	// init filter graph

	int ret = -1;

	filter_graph = avfilter_graph_alloc();

	ret = avfilter_graph_create_filter(&src_mic, avfilter_get_by_name("abuffer"), "src_mic",
		"sample_rate=44100:sample_fmt=s16:channel_layout=stereo", NULL, filter_graph);
	ret = avfilter_graph_create_filter(&src_sys, avfilter_get_by_name("abuffer"), "src_sys",
		"sample_rate=48000:sample_fmt=s16:channel_layout=stereo", NULL, filter_graph);
	ret = avfilter_graph_create_filter(&amerge, avfilter_get_by_name("amerge"), "amerge",
		"inputs=2", NULL, filter_graph);
	ret = avfilter_graph_create_filter(&pan, avfilter_get_by_name("pan"), "pan",
		"stereo|c0=c0+c2|c1=c1+c3", NULL, filter_graph);
	ret = avfilter_graph_create_filter(&aformat, avfilter_get_by_name("aformat"), "aformat",
		"sample_fmts=fltp", NULL, filter_graph);
	ret = avfilter_graph_create_filter(&sink, avfilter_get_by_name("abuffersink"), "sink",
		NULL, NULL, filter_graph);

	ret = avfilter_link(src_mic, 0, amerge, 0);
	ret = avfilter_link(src_sys, 0, amerge, 1);
	ret = avfilter_link(amerge, 0, pan, 0);
	ret = avfilter_link(pan, 0, aformat, 0);
	ret = avfilter_link(aformat, 0, sink, 0);

	//avfilter_link(src_cam, 0, sink, 0);

	ret = avfilter_graph_config(filter_graph, NULL);
}

void AudioMerger::slotMergeAudioLoop()
{
	
	int ret = -1;
	while (1)
	{
		AVFrame* frame_merge = av_frame_alloc();
		if (!(queue_aud_mic.isEmpty()))
		{
			AVFrame* frame_mic = queue_aud_mic.dequeue();
			mutex.lock();
			ret = av_buffersrc_add_frame(src_mic, frame_mic);
			mutex.unlock();
			//av_frame_free(&frame_mic);?
		}
		if (!(queue_aud_sys.isEmpty()))
		{
			AVFrame* frame_sys = queue_aud_sys.dequeue();
			mutex.lock();
			ret = av_buffersrc_add_frame(src_sys, frame_sys);
			mutex.unlock();
			//av_frame_free(&frame_sys);?
		}
		if (av_buffersink_get_frame(sink, frame_merge) >= 0)
			emit sigSendMergedAudioFrame(frame_merge);
		else av_frame_free(&frame_merge);
	}

}