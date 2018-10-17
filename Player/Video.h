/*
 * This file is part of class player.
 * 
 */


#pragma once

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <queue>
#include <thread>
#include <mutex>
#include <QtGui/QImage>
#include <QtCore/QString>
using namespace std;

enum CONTROL
{
	PLAY,
	PAUSE,
	STOP,
	SAVEPIC,
	FASTFORWARD,
	FASTBACKWARD
};
class Parse
{
public:
	Parse() {}
	~Parse() {}

	void ParseStream();

	void UnpacketThread();	//sub thread: package parse. to m_pkts
	void DecodeThread();	//sub thread: decoding pkt to frame. from m_pkts
	bool IsPacketReady();	//to make sure whether packet in queue.
	void DisplayThread();	//Render frames.	Maybe main thread, not need to create new sub thread.

private:
	void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
		char *filename);

private:
	//thread m_parse_th;		//for Parse thread.
	//thread m_decode_th;		//for Decode thread.

	mutex m_mutex;
	mutex m_mutex_frame;
	condition_variable m_cond_var;
	bool m_bpkt_pushed;
	bool m_bquit;
	bool m_control;
	CONTROL m_order;

	char *m_url;					//url for play.
	AVFormatContext *m_fmt_ctx;		//
	AVDictionary *m_fmt_opts;		//
	AVPacket *m_pkt;				//
	AVFrame *m_frame;				//
	AVFrame *m_disframe;
	AVCodecContext *m_avcodec_ctx;	//
	queue<AVPacket *> m_pkts;		//packets.
	queue<AVFrame *> m_frames;		//frames
	SwsContext *m_sws_ctx;			//scale context
	unsigned char *out_buffer;
};


class Video
{
public:
	Video();
	~Video();

	void VideoPlay();		//main thread: play video.

private:
	//void VideoControl();	//control video.

private:
	thread m_unpacket_th;		//for Parse thread.
	thread m_decode_th;		//for Decode thread.
};

