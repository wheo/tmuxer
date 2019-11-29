#ifndef _TSMUXER_H_
#define _TSMUXER_H_

#define MAX_NUM_STREAMS		9
#include "tspes.h"
typedef struct {
	int streams;

	int dump_en;
	int dump_freq;

	int prog_pid;
	int muxrate;
	int payload_size;

	es_s es[MAX_NUM_STREAMS];
}muxer_s;

typedef struct {
	int codec;		//> 0- H264, 1- HEVC
	int profile;
	int level;
	int quality;
	int resolution;
	int width;
	int height;
	int rc;
	int bitrate;
	double fps;
	int min_gop;
	int max_gop;
	int bframes;
	int refs;
	int ratio_x;
	int ratio_y;

	bool is_cabac;
	bool is_interlace;
	bool is_dummy1;
	bool is_dummy2;
}video_cfg_s;

typedef struct {
	int output;	//> 0- ts, 1- mp4
	int mux_rate;

	video_cfg_s vid;
}mux_cfg_s;

typedef struct {
	char strIP[100];
	int nPort;
	mux_cfg_s mux;
}channel_cfg_s;

typedef struct {
	uint8_t p[1328];
	int len;
	int corrupt;

	int64_t valid_pts;
	int64_t pcr;
}packet_s;

// a wrapper around a single output AVStream
typedef struct OutputStream {
	AVStream *st;
	AVCodecContext *enc;

	/* pts of the next frame that will be generated */
	int64_t next_pts;
	int samples_count;

	AVFrame *frame;
	AVFrame *tmp_frame;

	float t, tincr, tincr2;

	struct SwsContext *sws_ctx;
	struct SwrContext *swr_ctx;
} OutputStream;

class CTSMuxer
{
public:
	CTSMuxer();
	~CTSMuxer();

	bool CreateOutput(const char *strFilePath, mux_cfg_s *mux_cfg, int nRecSec = -1);
	void DeleteOutput();

	void put_data(unsigned char *pData, int nDataSize);
	void put_data(AVPacket *pkt);

private:
	void add_stream(OutputStream *ost, AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id);
	void close_stream(AVFormatContext *oc, OutputStream *ost);	

	void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
	void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);

	int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt);

	AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
protected:
	muxer_s m_muxer;

	int m_nRecSec;
private:
	bool m_bhave_video;
	bool m_bhave_audio;

	AVFormatContext *m_poc_ctx;
	OutputStream m_video_st;
	OutputStream m_audio_st;

	mux_cfg_s m_mux_cfg;

	AVCodec *m_audio_codec;
	AVCodec *m_video_codec;
	
	int64_t m_nFrameCount;
};

#endif