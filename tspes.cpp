#include "tspes.h"

#define MAX_PTS					8589934591

const uint8_t *avpriv_find_start_code(const uint8_t *p,
		const uint8_t *end,
		uint32_t *state)
{
	int i;

	if (p >= end)
		return end;

	for (i = 0; i < 3; i++) {
		uint32_t tmp = *state << 8;
		*state = tmp + *(p++);
		if (tmp == 0x100 || p == end)
			return p;
	}

	while (p < end) {
		if      (p[-1] > 1      ) p += 3;
		else if (p[-2]          ) p += 2;
		else if (p[-3]|(p[-1]-1)) p++;
		else {
			p++;
			break;
		}
	}

	p = FFMIN(p, end) - 4;
	#if 0
	*state = AV_RB32(p);
	#endif

	return p + 4;
}

CPES::CPES(void) {
	m_bIsHeader = true;

	m_nWritten = 0;
	memset(&m_pkt, 0, sizeof(pes_s));
}

CPES::~CPES(void) {
	pes_s *pp = &m_pkt;
	if (pp->p) {
		//_d("free > %x : %x\n", this, pp->p);
		free(pp->p);
	}
}

void CPES::Update() {
	pes_s *pp = &m_pkt;
	double fRatio = (double)m_nWritten/pp->size; // max 1.0
	pp->dts = pp->pts = m_nPTS + fRatio*pp->duration;
	if (pp->dts > MAX_PTS) {
		_d("[MUX] Rollover occur @ update (%ld)\n", pp->dts);
		pp->dts -= MAX_PTS;
		pp->pts = pp->dts;
	}
//	_d("[MUX] update pts %ld\n", pp->pts);
}

int64_t CPES::GetNextPTS() {
	pes_s *pp = &m_pkt;
	int64_t pts = m_nPTS + pp->duration;
	if (pts > MAX_PTS) {
		_d("[MUX] Rollover occur @ delete (%ld)\n", pts);
		pts -= MAX_PTS;
	}
	return pts;
}

void CPES::Set(es_s *pesi) {
	pes_s *pp = &m_pkt;

	pp->p = (uint8_t *)malloc(pesi->size);
	pp->size = pesi->size;
	pp->pts = pesi->pts;
	pp->dts = pesi->dts;
	pp->duration = pesi->duration;
	pp->flags = pesi->flags;
	pp->index = pesi->index;

	//_d("malloc > %s : %x\n", this, pp->p);
	m_nPTS = pp->pts;
	
	memcpy(pp->p, pesi->payload, pesi->size);
}

void CPES::Set(AVFormatContext *s, AVPacket *pkt) {
	AVStream *st = s->streams[pkt->stream_index];
	pes_s *pp = &m_pkt;

	int size = pkt->size;
	uint8_t *buf = pkt->data;
	uint8_t *data = NULL;
#if 0
	if (st->codecpar->codec_id == AV_CODEC_ID_H264) {
		const uint8_t *p = buf, *buf_end = p+size;
		uint32_t state = -1;

		int extadd = (pkt->flags & AV_PKT_FLAG_KEY) ? st->codecpar->extradata_size : 0;
		if (extadd && AV_RB24(st->codecpar->extradata) > 1) {
			extadd = 0;
		}
		do {
			p = avpriv_find_start_code(p, buf_end, &state);
			if ((state & 0x1f) == 7) {
				extadd = 0;
			}
		} while(p < buf_end && (state & 0x1f) != 9 && (state & 0x1f) != 5 && (state & 0x1f) != 1);
		if ((state & 0x1f) != 5) {
			extadd = 0;
		}
		if ((state & 0x1f) != 9) {
			size = pkt->size + 6 + extadd;

			pp->p = (uint8_t *)malloc(size);
			pp->size = size;

			data = pp->p;
			memcpy(data + 6, st->codecpar->extradata, extadd);
			memcpy(data + 6 + extadd, pkt->data, pkt->size);
			data[0] = 0x00;
			data[1] = 0x00;
			data[2] = 0x00;
			data[3] = 0x01;
			data[4] = 0x09;
			data[5] = 0xf0;
		} else {
			pp->p = (uint8_t *)malloc(pkt->size);
			pp->size = pkt->size;

			memcpy(pp->p, pkt->data, pkt->size);
		}
	} else if (st->codecpar->codec_id == AV_CODEC_ID_HEVC) {
		const uint8_t *p = buf, *buf_end = p+size;
		uint32_t state = -1;

		int extadd = (pkt->flags & AV_PKT_FLAG_KEY) ? st->codecpar->extradata_size : 0;
		if (extadd && AV_RB24(st->codecpar->extradata) > 1) {
			extadd = 0;
		}
		do {
			p = avpriv_find_start_code(p, buf_end, &state);
			if ((state & 0x7e) == 2*32) {
				extadd = 0;
			}
		} while(p < buf_end && (state & 0x7e) != 2*35 && (state & 0x7e) >= 2*32);

		if ((state & 0x7e) < 2*16 && (state & 0x7e) >= 2*24) {
			extadd = 0;
		}
		if ((state & 0x7e) != 2*35) {
			size = pkt->size + 7 + extadd;

			pp->p = (uint8_t *)malloc(size);
			pp->size = size;

			data = pp->p;
			memcpy(data + 7, st->codecpar->extradata, extadd);
			memcpy(data + 7 + extadd, pkt->data, pkt->size);

			data[0] = 0x00;
			data[1] = 0x00;
			data[2] = 0x00;
			data[3] = 0x01;
			data[4] = 2*35;
			data[5] = 1;
			data[6] = 0x50;
		} else {
			pp->p = (uint8_t *)malloc(pkt->size);
			pp->size = pkt->size;

			memcpy(pp->p, pkt->data, pkt->size);
		}
	} else {
		pp->p = (uint8_t *)malloc(pkt->size);
		pp->size = pkt->size;

		memcpy(pp->p, pkt->data, pkt->size);
	}
#else
	pp->p = (uint8_t *)malloc(pkt->size);
	pp->size = pkt->size;

	memcpy(pp->p, pkt->data, pkt->size);
#endif
	//_d("malloc > %x : %x\n", this, pp->p);
	pp->index = pkt->stream_index;
	pp->pts = pkt->pts;
	pp->dts = pkt->dts;
	pp->duration = pkt->duration;
	pp->flags = pkt->flags;
}

pes_s *CPES::Get() {
	return &m_pkt;
}

int CPES::WriteHeader(void *p, int nPID, int *pCC) {
	uint8_t *pd = (uint8_t *)p;

	int nCC = *pCC;

	pd[0] = 0x47;
	pd[1] = (0x0 << 7 | (m_bIsHeader ? 1 : 0) << 6 | 0x0 << 5 | ((nPID >> 8) & 0x1f));
	pd[2] = (nPID & 0xff);
	pd[3] = (0x0 << 6 | 0x0 << 4 | nCC);

	nCC++;
	if (nCC >= 16) {
		nCC = 0;
	}
	*pCC = nCC;

	return 4;
}

bool CPES::WriteVideoPES(void *p, int nRemain, int &nResidu) {
	int nPesHeader = 0;
	uint8_t ph[64];
	uint8_t *pd1 = ph;
	uint8_t *ps = (uint8_t *)m_pkt.p;

	pes_s *pp = &m_pkt;

	nResidu = 0;
	if (m_bIsHeader) {
		int pes_len = 5;
		uint8_t pts_dts_flag = 0x2;
		if (pp->dts != AV_NOPTS_VALUE && pp->pts != pp->dts) {
			pts_dts_flag = 0x3;
			pes_len = 10;
		}

		*pd1++ = 0x00;
		*pd1++ = 0x00;
		*pd1++ = 0x01;
		*pd1++ = 0xE0; // video type
		*pd1++ = 0x00;
		*pd1++ = 0x00;
		*pd1++ = (0x2 << 6 | // fixed
				  0x0 << 4 | // PES scrambling
				  0x0 << 3 | // PES priority
				  0x1 << 2 | // Data align
				  0x0 << 1 | // copyright
				  0x0);		 // original
		*pd1++ = (pts_dts_flag << 6 | //PTS_DTS_flag
				  0x0 << 5 | // EPCR flag
				  0x0 << 4 | // ES_rate flag
				  0x0 << 3 | // DSM trick mode flag
				  0x0 << 2 | // additional copy info
				  0x0 << 1 | // PES_crc flag
				  0x0);		 // PES extension flag
		*pd1++ = pes_len;

		*pd1++ = (pts_dts_flag << 4 | 	// PTS_DTS_flag
				  pp->pts >> 29 | 	// upper 3bits
				  0x1); 	  			// MARKER

		*pd1++ = ((pp->pts >> 22)&0xff);
		*pd1++ = ((pp->pts >> 14)&0xfe) | 0x1;
		*pd1++ = ((pp->pts >> 7)&0xff);
		*pd1++ = ((pp->pts << 1)&0xfe) | 0x1;

		if (pts_dts_flag == 0x3) {
			*pd1++ = (0x1 << 4 | (pp->dts >> 29) | 0x1);
			*pd1++ = ((pp->dts >> 22)&0xff);
			*pd1++ = ((pp->dts >> 14)&0xfe) | 0x1;
			*pd1++ = ((pp->dts >> 7)&0xff);
			*pd1++ = ((pp->dts << 1)&0xfe) | 0x1;
		}
		nPesHeader = (int)(pd1 - ph);
	}

	uint8_t *pd = (uint8_t *)p;
	if (nRemain) {
		int nDiff = pp->size - m_nWritten;
		if (m_bIsHeader) {
			nDiff += nPesHeader;
		}

		if ((nRemain - nDiff) == 1 || (nRemain - nDiff) == 2) {
			int w = 32;
			if (m_bIsHeader) {
				memcpy(&pd[w], ph, nPesHeader);
				w += nPesHeader;
			}
			memcpy(&pd[w], &ps[m_nWritten], (188 - w));
			m_nWritten += (188-w);
			nResidu = (188-w);
		} else if (nDiff < nRemain) { 
			nResidu = nDiff;
			if (m_bIsHeader) {
				memcpy(&pd[188-nDiff], ph, nPesHeader);
				nDiff -= nPesHeader;
			}
			memcpy(&pd[188-nDiff], &ps[m_nWritten], nDiff);
			m_nWritten += nDiff;
		} else {
			pd += (188 - nRemain);
			if (m_bIsHeader) {
				memcpy(pd, ph, nPesHeader);
				pd += nPesHeader;
				nRemain -= nPesHeader;
			}
			memcpy(pd, &ps[m_nWritten], nRemain);
			m_nWritten += nRemain;
		}
	}
	m_bIsHeader = false;
	if (m_nWritten >= pp->size) {
		return true;
	}

	return false;
}

bool CPES::WriteAudioPES(void *p, int nRemain, int &nResidu) {
	uint8_t *pd = (uint8_t *)p;
	uint8_t *ps = (uint8_t *)m_pkt.p;

	pes_s *pp = &m_pkt;

	if (m_bIsHeader) {
		int nPacketLen = pp->size + 8; // include PES header len
		uint8_t type = 0xBD; // AC3 : 0xBD, AAC : 0xC0
		if (ps[0] == 0xff && (ps[1] & 0xf0) == 0xf0) {
			type = 0xC0 + (pp->index-1);
		}

		*pd++ = 0x00;
		*pd++ = 0x00;
		*pd++ = 0x01;
		*pd++ = type;
		*pd++ = ((nPacketLen >> 8)&0xff);
		*pd++ = ((nPacketLen & 0xff));
		*pd++ = (0x2 << 6 | // fixed
				 0x0 << 4 | // PES scrambling
				 0x0 << 3 | // PES priority
				 0x1 << 2 | // Data align
				 0x0 << 1 | // copyright
				 0x0);		// original
		*pd++ = (0x2 << 6 | // PTS_DTS_flag
				 0x0 << 5 | // EPCR flag
				 0x0 << 4 | // ES_rate flag
				 0x0 << 3 | // DSM trick mode flag
				 0x0 << 2 | // additional copy info
				 0x0 << 1 | // PES_crc flag
				 0x0);		// PES extension flag
		*pd++ = 5; // PES header data length

		*pd++ = (0x2 << 4 | // PTS_DTS_flag
				 pp->pts >> 29 | // upper 3bits
				 0x1);		// MARKER

		*pd++ = ((pp->pts >> 22)&0xff);
		*pd++ = ((pp->pts >> 14)&0xfe) | 0x1;
		*pd++ = ((pp->pts >> 7)&0xff);
		*pd++ = ((pp->pts << 1)&0xfe) | 0x1;

		nRemain -= (int)(pd - (uint8_t*)p);
		m_bIsHeader = false;
	}

	nResidu = 0;
	if (nRemain) {
		int nLocalSize = 0;
		int nDiff = (pp->size - m_nWritten);
		if (nDiff < nRemain) { 
			// 183 < 184 이면 nResidu 는 1 이 경우 buff[5] = 0 이 되며 syntax error가 난다.
			// 따라서 nDiff가 183이면 93 + 90 처럼 둘로 나누자.
			if (nDiff == 183) {
				//_d("[MUX] AUDIO > cut the PAYLOAD two\n");
				nLocalSize = 93;
				nResidu = nRemain - 93;
			} else {
				nResidu = nRemain - nDiff;
				nLocalSize = nDiff;
			}
		} else {
			nLocalSize = nRemain;
		}

		memcpy(pd, &ps[m_nWritten], nLocalSize);
		m_nWritten += nLocalSize;
	}

	if (m_nWritten >= pp->size) {
		return true;
	}

	return false;
}

int PutPCR(void *p, uint64_t pcr_base, int pcr_ext, bool isDiscPCR, bool isIDR) {
	uint8_t *pd = (uint8_t *)p;

	pd[0] = 0x07; // ad field length
	pd[1] = (isDiscPCR << 7 | // discontinuity indicator
			 isIDR << 6 | // random access indicator
			 0x0 << 5 | // ES priority indicator
			 0x1 << 4 | // PCR flag
			 0x0 << 3 | // OPCR flag
			 0x0 << 2 | // splicing point flag
			 0x0 << 1 | // ts private data flag
			 0x0); 		// ad extension flag
	pd[2] = (pcr_base >> 25)&0xff;
	pd[3] = (pcr_base >> 17)&0xff;
	pd[4] = (pcr_base >> 9)&0xff;
	pd[5] = (pcr_base >> 1)&0xff;
	pd[6] = 0x7e | ((pcr_base&0x1) << 7 | ((pcr_ext >> 8) & 0x1));
	pd[7] = (pcr_ext & 0xff);

	return 8;
}

int PutPCROnly(void *p, uint64_t pcr_base, int pcr_ext, bool isDiscPCR) {
	uint8_t *pd = (uint8_t *)p;

	pd[0] = 0x07; // ad field length
	pd[1] = (isDiscPCR << 7 | // discontinuity indicator
			 0x0 << 6 | // random access indicator
			 0x0 << 5 | // ES priority indicator
			 0x1 << 4 | // PCR flag
			 0x0 << 3 | // OPCR flag
			 0x0 << 2 | // splicing point flag
			 0x0 << 1 | // ts private data flag
			 0x0); 		// ad extension flag
	pd[2] = (pcr_base >> 25)&0xff;
	pd[3] = (pcr_base >> 17)&0xff;
	pd[4] = (pcr_base >> 9)&0xff;
	pd[5] = (pcr_base >> 1)&0xff;
	pd[6] = 0x7e | ((pcr_base&0x1) << 7 | ((pcr_ext >> 8) & 0x1));
	pd[7] = (pcr_ext & 0xff);

	return 8;
}



