#include "main.h"
#include "misc.h"

std::string get_current_time_and_date() {
#if 0
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
#endif

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    std::stringstream ss;
    //ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d%H%M%S");
    ss << tm.tm_year + 1900 << tm.tm_mon + 1 << tm.tm_mday << tm.tm_hour << tm.tm_min << tm.tm_sec;
    return ss.str();
}

double rnd(double x, int digit) {
	return (floor((x)*pow(float(10), digit) + 0.5f)/pow(float(10), digit));
}

unsigned long GetTickCount() {
    timeval tv;
    static time_t sec = timeStart.tv_.tv_sec;
    static time_t usec = timeStart.tv_.tv_usec;

    gettimeofday(&tv, NULL);
    return (tv.tv_sec - sec)*1000 + (tv.tv_usec - usec)/1000;
}

PThread::PThread(char *a_szName , THREAD_EXIT_STATE a_eExitType) {
	m_nID = 0;
	m_eState = eREADY;
	m_eExitType = a_eExitType;
	m_szName = NULL;
	m_bExit = false;
	if (a_szName) {
		int nLength =strlen(a_szName);
		m_szName = new char[nLength + 1];
		strcpy(m_szName, a_szName);
	}
}

PThread::~PThread() {
	if (m_eState == eZOMBIE) {
		Join(m_nID);
	}

	if (m_szName)
		delete []m_szName;
}

int PThread::Start() {
	int nResult = 0;
	m_bExit = false;
	if ((pthread_create(&m_nID, NULL, StartPoint, reinterpret_cast<void *>(this))) == 0) {
		if (m_eExitType == eDETACHABLE) {
			pthread_detach(m_nID);
		}
	} else {
		SetState(eABORTED);
		nResult = -1;
	}

	return nResult;
}

void* PThread::StartPoint(void* a_pParam) {
	PThread* pThis = reinterpret_cast<PThread*>(a_pParam);
	pThis->SetState(eRUNNING);

	pThis->Run();

	if (pThis->GetExitType() == eDETACHABLE) {
		pThis->SetState(eTERMINATED);
	} else {
		pThis->SetState(eZOMBIE);
	}

	pthread_exit((void*)NULL);
	return NULL;
}

void PThread::Terminate() {
	if (m_eState == eRUNNING) {
		m_bExit = true;
		if (m_eExitType == eJOINABLE) {
			OnTerminate();
			Join(m_nID);
		} else if (m_eExitType == eDETACHABLE) {
			OnTerminate();
		}
	} else if (m_eState == eZOMBIE) {
		Join(m_nID);
	}
}

// this function will be blocked until the StartPoint terminates
void PThread::Join(pthread_t a_nID) {
	if (!pthread_join(a_nID, NULL)) {
		SetState(eTERMINATED);
	} else {
		printf("Failed to join thread [%s: %d]\n", __FUNCTION__, __LINE__);
	}
}

void PThread::SetState(THREAD_STATE a_eState) {
	m_eState = a_eState;
}

THREAD_STATE PThread::GetState() const {
	return m_eState;
}

THREAD_EXIT_STATE PThread::GetExitType() const {
	return m_eExitType;
}

char* PThread::GetName() const {
	return m_szName ? m_szName  : NULL;
}

bool PThread::IsTerminated() const {
	return m_eState == eTERMINATED  ?  true : false;
}

bool PThread::IsRunning() const {
	return m_eState == eRUNNING  ?  true : false;
}

