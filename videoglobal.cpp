#include "videoglobal.h"
#include <locale.h>

FILE *g_fpLog = NULL;

#if 0
CMyRing::CMyRing(int nMaxSize, int nChannel)
{
	m_pBase = new byte[nMaxSize];

	m_nRP = 0;
	m_nWP = 0;

	m_nWritten = 0;
	m_nRemain = nMaxSize;
	m_nTotal = nMaxSize;

	m_bExit = false;
	m_nChannel = nChannel;
}

CMyRing::~CMyRing()
{
	if (m_pBase) {
		delete[] m_pBase;
	}
}

int CMyRing::Read(uint8_t *buf, int buf_size, bool bBlock)
{
	while (1) {
		m_cs.Lock();
		if (m_nWritten > buf_size) {
			m_cs.Unlock();
			break;
		}
		m_cs.Unlock();

		if (bBlock == false) {
			return -1;
		}

		Sleep(2);	// 읽을 크기보다 적을 경우 대기
		if (m_bExit) {
			return 0;
		}
	}

	int nS1, nS2;
	int nRP = m_nRP, nWP = m_nWP;

	if (nRP >= nWP) {
		// 0=======W------R=======m_nTotal 의 경우(1)
		nS1 = m_nTotal - nRP;
		nS2 = nWP;
	}
	else {
		// 0-------R======W-------m_nTotal 의 경우(2)
		nS1 = nWP - nRP;
		nS2 = 0;
	}

	if (nS1 >= buf_size) {
		// 한쪽만으로도 충분한 데이터가 있는 경우(1,2)
		memcpy(buf, &m_pBase[nRP], buf_size);

		nRP += buf_size;
		if (nRP >= m_nTotal) {
			if (nRP > m_nTotal) {
				_d("Ring-Read read error msg\n");
			}
			nRP = 0;
		}
	}
	else {
		// 양쪽을 다 써야 하는 경우(1)
		//	_d("Ring-Read wrap case occur(written %d, read %d, size1 %d)\n", m_nWritten, buf_size, nS1);

		memcpy(buf, &m_pBase[nRP], nS1);
		memcpy(&buf[nS1], &m_pBase[0], buf_size - nS1);

		nRP = buf_size - nS1;
	}

	// 정보 업데이트
	// !!! 주의 !!! m_nRemain은 적절한 동기화를 위해 제일 나중에 바뀌어야 함
	m_cs.Lock();
	m_nRP = nRP;
	m_nWritten -= buf_size;
	m_nRemain += buf_size;
	m_cs.Unlock();

	if (m_nChannel == 1) {
		if (m_nWP == m_nRP) {
			_d("Ring-Read error remain: %d, write: %d\n", m_nRemain, m_nWritten);
		}
	}

	return buf_size;
}

int CMyRing::Write(uint8_t *buf, int buf_size)
{
	while (1) {
		m_cs.Lock();
		if (m_nRemain >= buf_size) {
			m_cs.Unlock();
			break;
		}
		m_cs.Unlock();

		Sleep(2);	// 남은 크기가 부족할 경우 대기
		if (m_bExit) {
			return 0;
		}

		_d("[CMyRing] 남은 크기가 부족할 경우 대기\n");
	}

	int nSize1, nSize2;
	int nRP = m_nRP, nWP = m_nWP;

	if (nWP >= nRP) {
		// ------R=======W------- 의 경우(1)
		nSize1 = m_nTotal - nWP;
		nSize2 = nRP;
	}
	else {
		// ======W-------R======= 의 경우(2)
		nSize1 = nRP - nWP;
		nSize2 = 0;
	}

	if (nSize1 >= buf_size) {
		// 한쪽만으로도 충분(1,2)
		memcpy(&m_pBase[nWP], buf, buf_size);

		nWP += buf_size;
		if (nWP >= m_nTotal) {
			if (nWP > m_nTotal) {
				_d("[CMyRing] Write error \n");
			}
			nWP = 0;
		}
	}
	else {
		// 양쪽이 다 필요(2)
		//	_d("Ring-Write wrap case occur(remain %d, write %d, size1 %d)\n", m_nRemain, buf_size, nSize1);

		memcpy(&m_pBase[nWP], buf, nSize1);
		memcpy(&m_pBase[0], &buf[nSize1], buf_size - nSize1);

		nWP = buf_size - nSize1;
	}

	// 업데이트
	// !!! 주의 !!! m_nWritten은 적절한 동기화를 위해 제일 나중에 바뀌어야 함
	m_cs.Lock();
	m_nWP = nWP;
	m_nRemain -= buf_size;
	m_nWritten += buf_size;
	m_cs.Unlock();

	if (m_nChannel == 1) {
		if (m_nWP == m_nRP) {
			_d("[CMyRing] Write error. remain: %d, write: %d\n", m_nRemain, m_nWritten);
		}
	}
	return buf_size;
}
#endif

CMyPacketPool::CMyPacketPool() {
	/*for (int i = 0; i < MAX_NUM_OF_PACKETS; i++) {
		m_pPkt[i] = &m_pkt[i];
	}*/

	for (int i = 0; i < MAX_NUM_OF_PACKETS; i++) {
		m_pPkt[i] = NULL;
	}

	m_nRead = 0;
	m_nWrite = 0;
	m_nRemain = MAX_NUM_OF_PACKETS;
}

CMyPacketPool::~CMyPacketPool() {
	for (int i = 0; i < MAX_NUM_OF_PACKETS; i++) {
		if (m_pPkt[i]) {
			av_packet_free(&m_pPkt[i]);
		}
	}
}

bool CMyPacketPool::Put(char *pData, int nSize, int64_t nTime) {
	if (m_pPkt[m_nWrite]) {
		return false;
	}

	AVPacket *pkt = av_packet_alloc();
	if (!pkt) {
		return false;
	}

	pkt->data = (u_int8_t*)pData;
	pkt->size = nSize;
	pkt->pts = nTime;

	m_pPkt[m_nWrite] = pkt;
	m_nWrite++;

	if (m_nWrite >= MAX_NUM_OF_PACKETS) {
		m_nWrite = 0;
	}

	return true;
}

AVPacket *CMyPacketPool::Get() {
	if (!m_pPkt[m_nRead]) {
		return NULL;
	}

	AVPacket *pkt = m_pPkt[m_nRead];
	
	m_pPkt[m_nRead] = NULL;
	m_nRead++;
	
	if (m_nRead >= MAX_NUM_OF_PACKETS) {
		m_nRead = 0;
	}

	return pkt;	

	/*AVPacket *pp = NULL;

	for (int i = 0; i < MAX_NUM_OF_PACKETS; i++) {
		pp = m_pPkt[i];
		if (pp) {
			m_pPkt[i] = NULL;

			return pp;
		}
	}

	return NULL;*/
}

void CMyPacketPool::Ret(AVPacket *pp) {
	for (int i = 0; i < MAX_NUM_OF_PACKETS; i++) {
		if (!m_pPkt[i]) {
			m_pPkt[i] = pp;
			break;
		}
	}
}