#include "pch.h"
#include "Video.h"


Video::Video()
{
}


Video::~Video()
{
}

void Video::VideoPlay()
{
	Parse parse_video;
	parse_video.ParseStream();
	m_decode_th = thread(&Parse::DecodeThread, &parse_video);
	m_unpacket_th = thread(&Parse::UnpacketThread, &parse_video);
	parse_video.DisplayThread();
	m_unpacket_th.join();
	m_decode_th.join();
	//this_thread::sleep_for(5s);
}

void Parse::ParseStream()
{
	m_url = (char*)"D:/work/Resource/oceans.mp4";
	avformat_network_init();
	int err;
	err = avformat_open_input(&m_fmt_ctx, m_url, nullptr, nullptr);
	if (err < 0)
		return;

	err = avformat_find_stream_info(m_fmt_ctx, nullptr);
	if (err < 0)
		return;

	av_dump_format(m_fmt_ctx, 0, m_url, 0);

	AVCodec *dec;
	int v_index = av_find_best_stream(m_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
	m_avcodec_ctx = avcodec_alloc_context3(dec);

	avcodec_parameters_to_context(m_avcodec_ctx, m_fmt_ctx->streams[v_index]->codecpar);

	m_avcodec_ctx->pkt_timebase = m_fmt_ctx->streams[v_index]->time_base;

	if ((err = avcodec_open2(m_avcodec_ctx, dec, nullptr) < 0))
	{
		printf_s("Cannot open video decoder.\n");
		return;
	}

	//
	AVStream *v_stream = m_fmt_ctx->streams[v_index];

	int srcW = m_avcodec_ctx->width;
	int srcH = m_avcodec_ctx->height;

	m_pkt = av_packet_alloc();
	m_frame = av_frame_alloc();
	m_disframe = av_frame_alloc();
	out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB32, m_avcodec_ctx->width, m_avcodec_ctx->height, 1));
	av_image_fill_arrays(m_disframe->data, m_disframe->linesize, out_buffer,
		AV_PIX_FMT_RGB32, m_avcodec_ctx->width, m_avcodec_ctx->height, 1);
	m_sws_ctx = sws_getContext(srcW, srcH, m_avcodec_ctx->pix_fmt, 
		srcW, srcH, AV_PIX_FMT_RGB32, SWS_BICUBIC, nullptr, nullptr, nullptr);

	
	
}

void Parse::UnpacketThread()
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

		if (m_pkt->stream_index == AVMEDIA_TYPE_VIDEO)
		{
			lock_guard<mutex> guard(m_mutex);
			AVPacket *pkt = av_packet_clone(m_pkt);
			m_pkts.push(pkt);
			//av_packet_unref(m_pkt);
			m_bpkt_pushed = true;
			m_cond_var.notify_one();
		}
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
		AVPacket *pkt = av_packet_alloc();
		av_packet_ref(pkt, m_pkts.front());
		m_pkts.pop();
		m_bpkt_pushed = false;
		
		ret = avcodec_send_packet(m_avcodec_ctx, pkt);
		if (ret < 0)
			continue;
		ret = avcodec_receive_frame(m_avcodec_ctx, m_frame);
		if (ret < 0)
			continue;

		AVFrame *frame = av_frame_clone(m_frame);
		m_frames.push(frame);
		//av_frame_unref(m_frame);
	}
}

bool Parse::IsPacketReady()
{
	return m_bpkt_pushed;
}

void Parse::DisplayThread()
{
	for (;;)
	{
		if (m_control)
		{
			switch (m_order)
			{
			case PLAY:
				break;
			default:
				break;
			}
		}
		else
		{
			av_usleep((int64_t)30 * 1000);
			lock_guard<mutex> guard(m_mutex_frame);
			if (m_frames.size() <= 0)
			{
				continue;
			}
			av_frame_ref(m_frame, m_frames.front());
			//6m_disframe = m_frames.front();
			m_frames.pop();

			sws_scale(m_sws_ctx, (const uint8_t * const *)m_frame->data, m_frame->linesize,
				0, m_disframe->height, m_disframe->data, m_disframe->linesize);

			QImage tmpImg((uchar *)out_buffer, m_avcodec_ctx->width, m_avcodec_ctx->height, QImage::Format_RGB32);
			int err = tmpImg.save("test.png");
		}
	}
	/*
	for(;;)
	{
		if(controls)
		{
			video control;
		}
		else
		{
			if(delay > 0)
				sleep(delay);
			reset delay;
			display frame and caculate delay;
		}
	}
	*/
}
