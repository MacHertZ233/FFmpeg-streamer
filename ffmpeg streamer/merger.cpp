#include "merger.h"

VideoMerger::VideoMerger()
{
	// init filter graph

	int ret = -1;

	filter_graph = avfilter_graph_alloc();

	ret = avfilter_graph_create_filter(&src_cam, avfilter_get_by_name("buffer"), "src_cam",
		"video_size=640x360:pix_fmt=2:time_base=1/60:pixel_aspect=1/1", NULL, filter_graph);
	ret = avfilter_graph_create_filter(&src_scr, avfilter_get_by_name("buffer"), "src_scr",
		"video_size=1920x1080:pix_fmt=2:time_base=1/60:pixel_aspect=1/1", NULL, filter_graph);
	ret = avfilter_graph_create_filter(&overlay, avfilter_get_by_name("overlay"), "overlay",
		"x=0:y=0:format=rgb", NULL, filter_graph);
	ret = avfilter_graph_create_filter(&sink, avfilter_get_by_name("buffersink"), "sink",
		NULL, NULL, filter_graph);


	ret = avfilter_link(src_scr, 0, overlay, 0);
	ret = avfilter_link(src_cam, 0, overlay, 1);
	ret = avfilter_link(overlay, 0, sink, 0);

	//avfilter_link(src_cam, 0, sink, 0);

	ret = avfilter_graph_config(filter_graph, NULL);

}

void VideoMerger::slotMergeVideoLoop()
{
	int ret = -1;
	while (1)
	{
		if (!(queue_vid_cam.isEmpty() || queue_vid_scr.isEmpty()))
		{
			//AVFrame* frame_cam, * frame_scr;

			mutex.lock();
			ret = av_buffersrc_add_frame(src_cam, queue_vid_cam.dequeue());
			ret = av_buffersrc_add_frame(src_scr, queue_vid_scr.dequeue());
			mutex.unlock();
			
			AVFrame* frame_merge = av_frame_alloc();
			ret = av_buffersink_get_frame(sink, frame_merge);

			emit sigSendMergedVideoFrame(frame_merge);
		}
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
	ret = avfilter_graph_create_filter(&sink, avfilter_get_by_name("abuffersink"), "sink",
		NULL, NULL, filter_graph);


	ret = avfilter_link(src_mic, 0, amerge, 0);
	ret = avfilter_link(src_sys, 0, amerge, 1);
	ret = avfilter_link(amerge, 0, pan, 0);
	ret = avfilter_link(pan, 0, sink, 0);

	//avfilter_link(src_cam, 0, sink, 0);

	ret = avfilter_graph_config(filter_graph, NULL);
}

void AudioMerger::slotMergeAudioLoop()
{
	AVFrame* frame_merge = av_frame_alloc();
	int ret = -1;
	while (1)
	{
		if (!(queue_aud_mic.isEmpty()))
		{
			mutex.lock();
			ret = av_buffersrc_add_frame(src_mic, queue_aud_mic.dequeue());
			mutex.unlock();
		}
		if (!(queue_aud_sys.isEmpty()))
		{
			mutex.lock();
			ret = av_buffersrc_add_frame(src_sys, queue_aud_sys.dequeue());
			mutex.unlock();
		}
		if(av_buffersink_get_frame(sink, frame_merge)>=0)
			emit sigSendMergedAudioFrame(frame_merge);
	}

}