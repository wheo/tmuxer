#ifndef _RECV_H_
#define _RECV_H_

#include "tsmuxer.h"
#include "videoglobal.h"

class CRecv : public PThread {
public:
	CRecv(void);
	~CRecv(void);

	bool send_bitstream(uint8_t *stream, int size);
	
	int ReadSocket(uint8_t * buffer, unsigned bufferSize);
	bool Create(Json::Value info, Json::Value attr, int nChannel);
	void Delete();

	bool Receive();

	void log(int type, int state);

protected:	
	int m_nChannel; // 현재 채널 넘버	
	Json::Value m_info; // 채널 정보 json
	Json::Value m_attr; // 채널 공유 속성 attribute

	char m_strShmPrefix[32];
	int m_nRead;
	int m_nWrite;

	pthread_mutex_t m_mutex_recv;
private:
	//mux_cfg_s m_mux_cfg;

	int m_nRecSec; // 얼마나 녹화를 할 것인가
	int m_nFrameCount; // 프레임 수
	int m_file_idx; // 파일 인덱스 번호
	string m_filename;

	Json::Value json;

	CTSMuxer *m_pMuxer;
	CMyPacketPool	*m_pPktPool;

protected:

	void Run();
	void OnTerminate() {};

};

extern CRecv *ipc;

#endif // _RECV