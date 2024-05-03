#include "merger.h"

VideoMerger::VideoMerger(int width, int height, int position)
{
    
    int ret = -1;

    QString args_buffer = "video_size=" + QString::number(width) 
        + "x"+ QString::number(height)
        + ":pix_fmt=2:time_base=1/30:frame_rate=30:pixel_aspect=1/1";

    QString args_overlay = ":format = rgb";
    switch (position)
    {
    case 0:
        args_overlay = "x=0:y=0" + args_overlay;
        break;
    case 1:
        args_overlay = "x=0:y=H-h" + args_overlay;
        break;
    case 2:
        args_overlay = "x=W-w:y=0" + args_overlay;
        break;
    case 3:
        args_overlay = "x=W-w:y=H-h" + args_overlay;
        break;
    }

    // init filter graph
    filter_graph = avfilter_graph_alloc();

    ret = avfilter_graph_create_filter(&src_cam, avfilter_get_by_name("buffer"), "src_cam",
        args_buffer.toStdString().data(), NULL, filter_graph);
    ret = avfilter_graph_create_filter(&src_scr, avfilter_get_by_name("buffer"), "src_scr",
        "video_size=1920x1080:pix_fmt=2:time_base=1/30:frame_rate=30:pixel_aspect=1/1", NULL, filter_graph);
    ret = avfilter_graph_create_filter(&overlay, avfilter_get_by_name("overlay"), "overlay",
        args_overlay.toStdString().data(), NULL, filter_graph);
    /*ret = avfilter_graph_create_filter(&fps, avfilter_get_by_name("libplacebo"), "libplacebo",
        "fps=60:frame_mixer=mitchell_clamp", NULL, filter_graph);*/
    ret = avfilter_graph_create_filter(&sink, avfilter_get_by_name("buffersink"), "sink",
        NULL, NULL, filter_graph);

    ret = avfilter_link(src_scr, 0, overlay, 0);
    ret = avfilter_link(src_cam, 0, overlay, 1);
    ret = avfilter_link(overlay, 0, sink, 0);

    //avfilter_link(src_scr, 0, sink, 0);

    ret = avfilter_graph_config(filter_graph, NULL);

    loop = 1;
}

void VideoMerger::slotMergeVideoLoop()
{
    int ret = -1;
    while (loop)
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

            frame_merge->pkt_duration = frame_cam->pkt_duration;
            frame_merge->pkt_dts = frame_cam->pkt_dts;

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

void VideoMerger::endMerge() { loop = 0; }

VideoMerger::~VideoMerger()
{
    queue_vid_cam.empty();
    queue_vid_scr.empty();
    
    avfilter_graph_free(&filter_graph);
}

AudioMerger::AudioMerger(double weight)
{
    // init filter graph

    int ret = -1;

    filter_graph = avfilter_graph_alloc();

    QString weight1 = QString::number(weight*2);
    QString weight2 = QString::number(2.00-weight*2);
    QString args_pan = "stereo|c0=" + weight1 + "*c0+" + weight2 + "*c2|c1=" + weight1 + "*c1+" + weight2 + "*c3";

    ret = avfilter_graph_create_filter(&src_mic, avfilter_get_by_name("abuffer"), "src_mic",
        "sample_rate=44100:sample_fmt=s16:channel_layout=stereo", NULL, filter_graph);
    ret = avfilter_graph_create_filter(&src_sys, avfilter_get_by_name("abuffer"), "src_sys",
        "sample_rate=48000:sample_fmt=s16:channel_layout=stereo", NULL, filter_graph);
    ret = avfilter_graph_create_filter(&amerge, avfilter_get_by_name("amerge"), "amerge",
        "inputs=2", NULL, filter_graph);
    ret = avfilter_graph_create_filter(&pan, avfilter_get_by_name("pan"), "pan",
        args_pan.toStdString().data(), NULL, filter_graph);
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

    loop = 1;
}

void AudioMerger::slotMergeAudioLoop()
{
    
    int ret = -1;
    while (loop)
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

void AudioMerger::endMerge() { loop = 0; }

AudioMerger::~AudioMerger()
{
    queue_aud_mic.empty();
    queue_aud_sys.empty();

    avfilter_graph_free(&filter_graph);
}