#pragma once

#include <chrono>
#include "main.h"

using namespace std;
using namespace chrono;

#define MAX_WIDTH	1920
#define MAX_HEIGHT	1080

/** some MACROs */
#define SAFE_DELETE(x) if (x) {delete x; }; x = NULL;
#define SAFE_RELEASE(x) if (x) { x->Release();}; x = NULL;
#define SAFE_FCLOSE(x) if (x) { fclose(x); } x = NULL;

#define rnd(x, digit) (floor((x)*pow(float(10), digit) + 0.5f) / pow(float(10), digit));

class CMyWarkingTime
{
public:
	CMyWarkingTime(void);
	~CMyWarkingTime(void);

	void SetStart();
	void SetEnd();
	int GetDuration();

protected:
	system_clock::time_point m_start;
	system_clock::time_point m_end;
};

/** Thread class */
enum MT_STATE {
	mtReady, mtRunning, mtTerminated, mtZombie, mtAborted
};


class CWorkingTime
{
public:
	CWorkingTime(void);
	~CWorkingTime(void);

	void SetStart();
	void SetEnd();
	int GetDuration();

protected:
	system_clock::time_point m_start;
	system_clock::time_point m_end;
};

#if 0
/** 아주 간단한 RingBuffer 클래스 */
class CMyRing
{
public:
	/**생성자
	@param nMaxSize : 최대 링버퍼 사이즈*/
	CMyRing::CMyRing(int nMaxSize, int nChannel = 0);
	CMyRing::~CMyRing();

	/**블럭모드를 해제한다.(쓰레드에서 원할한 종료를 위해) */
	void SetExit() { m_bExit = true; };

	/**링버퍼에 넣는다
	@param buf : 넣을 데이터가 들어있는 버퍼 포인터
	@param buf_size : 넣을 데이터의 사이즈
	@return 실제 쓴 크기*/
	int Write(uint8_t *buf, int buf_size);

	/**링버퍼에서 읽는다.
	@param buf : 읽어올 버퍼 포인터
	@param buf_size : 읽어올 사이즈
	@return 실제 읽은 크기*/
	int Read(uint8_t *buf, int buf_size, bool bBlock = true);
protected:
	int		m_nChannel; ////> 채널번호

	char	*m_pBase;	//> 링버퍼의 시작 주소

	bool	m_bExit;	//> 종료플래그(블럭모드해제)

	int		m_nRP;		//> Read Position
	int		m_nWP;		//> Write Position

	int		m_nRemain;	//> 링버퍼의 남은 크기
	int		m_nWritten;	//> 링버퍼에 쓴 크기
	int		m_nTotal;	//> 전체 크기

	CCriticalSection m_cs; 
};
#endif

#define MAX_NUM_OF_PACKETS			65536
class CMyPacketPool
{
public:
	CMyPacketPool();
	~CMyPacketPool();

	bool Put(char *pData, int nSize, int64_t nTime);

	/* 빈 패킷 구조체 포인터를 반환한다.
	@return AVPacket 구조체 포인터 */
	AVPacket *Get();

	/* 다 사용한 AVPacket 구조체 포인터를 풀에 반환한다.
	@param pp : AVPacket 구조체 포인터 */
	void Ret(AVPacket *pp);

protected:

	AVPacket	*m_pPkt[MAX_NUM_OF_PACKETS];
	//AVPacket	m_pkt[MAX_NUM_OF_PACKETS];

	int			m_nRemain;

	int m_nRead;
	int m_nWrite;
};