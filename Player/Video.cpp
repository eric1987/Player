#include "pch.h"
#include "Video.h"


Video::Video()
{
}


Video::~Video()
{
}

void Video::VideoInit()
{
	int err;
	err = avformat_open_input(&m_fmt_ctx, m_url, nullptr, nullptr);
	if (err < 0)
		return;

	err = avformat_find_stream_info(m_fmt_ctx, nullptr);
	if (err < 0)
		return;

	AVCodec *dec;
	int v_index = av_find_best_stream(m_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
	m_vctx = avcodec_alloc_context3(dec);

	avcodec_parameters_to_context(m_vctx, m_fmt_ctx->streams[v_index]->codecpar);

	m_vctx->pkt_timebase = m_fmt_ctx->streams[v_index]->time_base;

	if ((err = avcodec_open2(m_vctx, dec, nullptr) < 0))
	{
		printf_s("Cannot open video decoder.\n");
		return;
	}

	//
	AVStream *v_stream = m_fmt_ctx->streams[v_index];

}

void Video::VideoPlay()
{
	VideoInit();
	Parse parse(m_vctx);

	m_decode_th = thread(&Parse::ParseThread, &parse);
	m_parse_th = thread(&Parse::DecodeThread, &parse);
}

void Parse::ParseThread()
{
	for (;;)
	{
		if (m_bquit)
			break;
		int ret = av_read_frame(m_fmt_ctx, m_pkt);
		if (ret < 0)
		{
			continue;
		}

		lock_guard<mutex> guard(m_mutex);
		m_pkts.push(m_pkt);
		av_packet_unref(m_pkt);
		m_bpkt_pushed = true;
		m_cond_var.notify_one();
	}
}

void Parse::DecodeThread()
{
	for (;;)
	{
		int ret;
		if (m_bquit)
			break;
		unique_lock<std::mutex> mlock(m_mutex);
		m_cond_var.wait(mlock, bind(&Parse::IsPacketReady, this));
		AVPacket *pkt = m_pkts.front();
		m_bpkt_pushed = false;
		
		ret = avcodec_send_packet(m_avcodec_ctx, pkt);
		if (ret < 0)
			continue;
		ret = avcodec_receive_frame(m_avcodec_ctx, m_frame);
		if (ret < 0)
			continue;

		m_frames.push(m_frame);
		av_frame_unref(m_frame);
	}
}

bool Parse::IsPacketReady()
{
	return m_bpkt_pushed;
}
