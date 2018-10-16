/*
 * This file is part of class player.
 *
 */


#pragma once

extern "C"
{
#include <libavformat/avformat.h>
}

#include <queue>
#include <thread>
#include <mutex>
using namespace std;

class Parse
{
public:
	Parse(AVCodecContext *codec_ctx) 
	{
		m_avcodec_ctx = codec_ctx;
	}
	~Parse() {}

	void ParseThread();		//sub thread: package parse. to m_pkts
	void DecodeThread();	//sub thread: decoding pkt to frame. from m_pkts
	bool IsPacketReady();	//to make sure whether packet in queue.

private:
	mutex m_mutex;
	condition_variable m_cond_var;
	bool m_bpkt_pushed;
	bool m_bquit;

	AVFormatContext *m_fmt_ctx;
	AVPacket *m_pkt;
	AVFrame *m_frame;
	AVCodecContext *m_avcodec_ctx;
	queue<AVPacket *> m_pkts;	//packets.
	queue<AVFrame *> m_frames;	//frames
};


class Video
{
public:
	Video();
	~Video();

	void VideoInit();
	void VideoPlay();		//main thread: play video.

private:
	void VideoDisplay();	//Render frames.
	void VideoControl();	//control video.

private:
	
	//queue<AVFrame> m_frames;//frames to render.
	
	thread m_parse_th;		//for Parse thread.
	thread m_decode_th;		//for Decode thread.

	char *m_url;			//url for play.
	AVFormatContext *m_fmt_ctx;	//
	AVDictionary *m_fmt_opts;	//

	AVCodecContext *m_vctx;	//
};

