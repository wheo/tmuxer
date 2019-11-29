#include "tsmuxer.h"

#define ENABLE_AUDIO	0

#define STREAM_DURATION   10.0
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

#define SCALE_FLAGS SWS_BICUBIC

CTSMuxer::CTSMuxer() {
	memset(&m_muxer, 0, sizeof(muxer_s));

	m_poc_ctx = NULL;

	m_bhave_video = false;
	m_bhave_audio = false;

	m_audio_codec = NULL;
	m_video_codec = NULL;

	m_video_st = { 0, };
	m_audio_st = { 0, };

	m_nFrameCount = 0;
}

CTSMuxer::~CTSMuxer() {
	DeleteOutput();
}

bool CTSMuxer::CreateOutput(const char* strFilePath, mux_cfg_s *mux_cfg, int nRecSec) {
	DeleteOutput();

	m_nRecSec = nRecSec;

	AVOutputFormat *fmt;
	AVDictionary *opt = NULL;
	int ret;

	memcpy(&m_mux_cfg, mux_cfg, sizeof(mux_cfg_s));
	
	/* allocate the output media context */
	avformat_alloc_output_context2(&m_poc_ctx, NULL, NULL, strFilePath);	
	if (!m_poc_ctx) {
		_d("[Muxer] Could not deduce output format from file extension: using MPEG.\n");
		avformat_alloc_output_context2(&m_poc_ctx, NULL, "mpeg", strFilePath);
	}

	if (!m_poc_ctx)
		return false;

	fmt = m_poc_ctx->oformat;

//	av_register_output_format(fmt);

	/* Add the audio and video streams using the default format codecs
	* and initialize the codecs. */
	if (fmt->video_codec != AV_CODEC_ID_NONE) {
		if (m_mux_cfg.vid.codec == 0) {
			fmt->video_codec = AV_CODEC_ID_H264;
		}
		else if (m_mux_cfg.vid.codec == 1) {
			fmt->video_codec = AV_CODEC_ID_HEVC;
		}
		add_stream(&m_video_st, m_poc_ctx, &m_video_codec, fmt->video_codec);
		m_bhave_video = true;
	//	encode_video = 1;
	}
#if ENABLE_AUDIO
	if (fmt->audio_codec != AV_CODEC_ID_NONE) {
		add_stream(&m_audio_st, m_poc_ctx, &m_audio_codec, fmt->audio_codec);
		m_bhave_audio = true;
	//	encode_audio = 1;
	}
#endif

	/* Now that all the parameters are set, we can open the audio and
	* video codecs and allocate the necessary encode buffers. */
	if (m_bhave_video)
		open_video(m_poc_ctx, m_video_codec, &m_video_st, opt);

	if (m_bhave_audio)
		open_audio(m_poc_ctx, m_audio_codec, &m_audio_st, opt);

	av_dump_format(m_poc_ctx, 0, strFilePath, 1);

	/* open the output file, if needed */
	if (!(fmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&m_poc_ctx->pb, strFilePath, AVIO_FLAG_WRITE);
		if (ret < 0) {
			//_d("[Muxer] Could not open '%s': %s\n", filename, av_err2str(ret));
			_d("[Muxer] Could not open %s\n", strFilePath);
			return 1;
		}
	}

	/* Write the stream header, if any. */
	ret = avformat_write_header(m_poc_ctx, &opt);
	if (ret < 0) {
		//fprintf(stderr, "Error occurred when opening output file: %s\n", av_err2str(ret));
		_d("[Muxer] Error occurred when opening output file: %s\n", strFilePath);
		return 1;
	}

	m_nFrameCount = 0;

	return true;
}

/* Add an output stream. */
void CTSMuxer::add_stream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id) {
	AVCodecContext *c;
	int i;

	/* find the encoder */
	*codec = avcodec_find_encoder(codec_id);
	if (!(*codec)) {
		_d("[Muxer] Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
		return;
	}
		
	ost->st = avformat_new_stream(oc, *codec);
	if (!ost->st) {
		_d("[Muxer] Could not allocate stream\n");
		return;
	}
	ost->st->id = oc->nb_streams - 1;
		
	c = avcodec_alloc_context3(*codec);
	if (!c) {
		_d("[Muxer] Could not alloc an encoding context\n");
		return;
	}
	ost->enc = c;

	switch ((*codec)->type) {
	case AVMEDIA_TYPE_AUDIO:
		c->sample_fmt = (*codec)->sample_fmts ?
			(*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
		c->bit_rate = 64000;
		c->sample_rate = 44100;
		if ((*codec)->supported_samplerates) {
			c->sample_rate = (*codec)->supported_samplerates[0];
			for (i = 0; (*codec)->supported_samplerates[i]; i++) {
				if ((*codec)->supported_samplerates[i] == 44100)
					c->sample_rate = 44100;
			}
		}
		c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
		c->channel_layout = AV_CH_LAYOUT_STEREO;
		if ((*codec)->channel_layouts) {
			c->channel_layout = (*codec)->channel_layouts[0];
			for (i = 0; (*codec)->channel_layouts[i]; i++) {
				if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
					c->channel_layout = AV_CH_LAYOUT_STEREO;
			}
		}
		c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
		//		ost->st->time_base = (AVRational) { 1, c->sample_rate };
		ost->st->time_base.num = 1;
		ost->st->time_base.den = c->sample_rate;
		break;

	case AVMEDIA_TYPE_VIDEO:
		c->codec_id = codec_id;
				
		c->bit_rate = m_mux_cfg.vid.bitrate;
		/* Resolution must be a multiple of two. */
		c->width = m_mux_cfg.vid.width;
		c->height = m_mux_cfg.vid.height;

		/* timebase: This is the fundamental unit of time (in seconds) in terms
		* of which frame timestamps are represented. For fixed-fps content,
		* timebase should be 1/framerate and timestamp increments should be
		* identical to 1. */
		//	ost->st->time_base = (AVRational) { 1, STREAM_FRAME_RATE };		
		if (m_mux_cfg.vid.fps == 29.97) {
			ost->st->time_base.num = 1001;
			ost->st->time_base.den = 30000;			
		}
		else {
			ost->st->time_base.num = 1;
			ost->st->time_base.den = m_mux_cfg.vid.fps;
		}
		c->time_base = ost->st->time_base;

		c->gop_size = m_mux_cfg.vid.max_gop; /* emit one intra frame every twelve frames at most */

		c->pix_fmt = STREAM_PIX_FMT;
	//	c->pix_fmt = AV_PIX_FMT_GRAY8;
	//	ost->enc->pix_fmt = AV_PIX_FMT_GRAY8;
		
		if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
			/* just for testing, we also add B-frames */
			c->max_b_frames = 2;
		}
		if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
			/* Needed to avoid using macroblocks in which some coeffs overflow.
			* This does not happen with normal video, it just happens here as
			* the motion of the chroma plane does not match the luma plane. */
			c->mb_decision = 2;
		}
		break;

	default:
		break;
	}
	
	/* Some formats want stream headers to be separate. */
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

void CTSMuxer::open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg) {
	int ret;
	AVCodecContext *c = ost->enc;
	AVDictionary *opt = NULL;

	av_dict_copy(&opt, opt_arg, 0);

	/* open the codec */
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		_d("[Muxer] Could not open video codec. %d\n", ret);
		//fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
		return;
	}

	/* allocate and init a re-usable frame */
	ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
	if (!ost->frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		return;
	}

	/* If the output format is not YUV420P, then a temporary YUV420P
	* picture is needed too. It is then converted to the required
	* output format. */
	ost->tmp_frame = NULL;
	if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
		ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
		if (!ost->tmp_frame) {
			fprintf(stderr, "Could not allocate temporary picture\n");
			exit(1);
		}
	}

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0) {
		fprintf(stderr, "Could not copy the stream parameters\n");
		exit(1);
	}	
}

/**************************************************************/
/* video output */

AVFrame *CTSMuxer::alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
	AVFrame *picture;
	int ret;

	picture = av_frame_alloc();
	if (!picture)
		return NULL;

	picture->format = pix_fmt;
	picture->width = width;
	picture->height = height;

	/* allocate the buffers for the frame data */
	ret = av_frame_get_buffer(picture, 32);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate frame data.\n");
		exit(1);
	}

	return picture;
}

void CTSMuxer::put_data(unsigned char *pData, int nDataSize) {
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = pData;
	pkt.size = (int)nDataSize;

	AVCodecContext *c = m_video_st.enc;
	pkt.pts = m_nFrameCount;
	pkt.dts = m_nFrameCount;
	
	write_frame(m_poc_ctx, &c->time_base, m_video_st.st, &pkt);

	m_nFrameCount++;
}
void CTSMuxer::put_data(AVPacket *pkt) {
	AVCodecContext *c = m_video_st.enc;
	pkt->pts = m_nFrameCount;
	pkt->dts = m_nFrameCount;

	write_frame(m_poc_ctx, &c->time_base, m_video_st.st, pkt);

	m_nFrameCount++;
}

int CTSMuxer::write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt) {
	int ret = 0;	
	/* rescale output packet timestamp values from codec to stream timebase */	
//	pkt->pts = av_rescale(g_frame_count, 12800, 25);	
//	pkt->pts = av_compare_ts(g_frame_count, m_video_st.enc->time_base, STREAM_DURATION, )
	av_packet_rescale_ts(pkt, *time_base, st->time_base);
	pkt->stream_index = st->index;
		
#if 0
	static __int64 pts = 0;
	pts += 1001;

	pkt->pts = pts;
	pkt->dts = pts;
#else
	pkt->dts = pkt->pts;
#endif
	
	/* Write the compressed frame to the media file. */
//	log_packet(fmt_ctx, pkt);
	ret = av_interleaved_write_frame(fmt_ctx, pkt);
	return ret;
}

void CTSMuxer::DeleteOutput() {
	if (m_poc_ctx) {
		AVOutputFormat *fmt;
		fmt = m_poc_ctx->oformat;

		/* Write the trailer, if any. The trailer must be written before you
		* close the CodecContexts open when you wrote the header; otherwise
		* av_write_trailer() may try to use memory that was freed on
		* av_codec_close(). */
		av_write_trailer(m_poc_ctx);		

		/* Close each codec. */		
		if (m_bhave_video)
			close_stream(m_poc_ctx, &m_video_st);
		if (m_bhave_audio)
			close_stream(m_poc_ctx, &m_audio_st);

		if (!(fmt->flags & AVFMT_NOFILE))
			/* Close the output file. */
			avio_closep(&m_poc_ctx->pb);
		
		/* free the stream */
		avformat_free_context(m_poc_ctx);
	}

	m_bhave_video = false;
	m_bhave_audio = false;
}

void CTSMuxer::close_stream(AVFormatContext *oc, OutputStream *ost) {
	avcodec_free_context(&ost->enc);
	av_frame_free(&ost->frame);
	av_frame_free(&ost->tmp_frame);
	sws_freeContext(ost->sws_ctx);
	swr_free(&ost->swr_ctx);
}

void CTSMuxer::open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg) {
#if 0
	AVCodecContext *c;
	int nb_samples;
	int ret;
	AVDictionary *opt = NULL;

	c = ost->enc;

	/* open it */
	av_dict_copy(&opt, opt_arg, 0);
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		fprintf(stderr, "Could not open audio codec: %s\n", av_err2str(ret));
		exit(1);
	}

	/* init signal generator */
	ost->t = 0;
	ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
	/* increment frequency by 110 Hz per second */
	ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

	if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
		nb_samples = 10000;
	else
		nb_samples = c->frame_size;

	ost->frame = alloc_audio_frame(c->sample_fmt, c->channel_layout,
		c->sample_rate, nb_samples);
	ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
		c->sample_rate, nb_samples);

	/* copy the stream parameters to the muxer */
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if (ret < 0) {
		fprintf(stderr, "Could not copy the stream parameters\n");
		exit(1);
	}

	/* create resampler context */
	ost->swr_ctx = swr_alloc();
	if (!ost->swr_ctx) {
		fprintf(stderr, "Could not allocate resampler context\n");
		exit(1);
	}

	/* set options */
	av_opt_set_int(ost->swr_ctx, "in_channel_count", c->channels, 0);
	av_opt_set_int(ost->swr_ctx, "in_sample_rate", c->sample_rate, 0);
	av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
	av_opt_set_int(ost->swr_ctx, "out_channel_count", c->channels, 0);
	av_opt_set_int(ost->swr_ctx, "out_sample_rate", c->sample_rate, 0);
	av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt", c->sample_fmt, 0);

	/* initialize the resampling context */
	if ((ret = swr_init(ost->swr_ctx)) < 0) {
		fprintf(stderr, "Failed to initialize the resampling context\n");
		exit(1);
	}
#endif
}