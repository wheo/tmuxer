#ifndef _MISC_H_
#define _MISC_H_

std::string get_current_time_and_date();
extern double rnd(double x, int digit);

namespace
{
    class __GET_TICK_COUNT
    {           
    public:             
        __GET_TICK_COUNT()          
        {                                       
            if (gettimeofday(&tv_, NULL) != 0) {                
                throw 0;                                                            
            }                                                                                   
        }                                                               
        timeval tv_;                        
    };                                          
    __GET_TICK_COUNT timeStart;
}

extern unsigned long GetTickCount();

enum THREAD_STATE {
        eREADY, eRUNNING, eTERMINATED, eZOMBIE, eABORTED,
};
enum THREAD_EXIT_STATE {
        eJOINABLE, eDETACHABLE,
};

class PThread {
public:
        PThread(char *a_szName = NULL, THREAD_EXIT_STATE a_eExitType = eJOINABLE);
        virtual ~PThread();

        int Start();
        void Terminate();

        THREAD_STATE GetState() const;
        THREAD_EXIT_STATE GetExitType() const;
        char* GetName() const;

        bool IsTerminated() const;
        bool IsRunning() const;

        bool m_bExit;
protected:
        static void* StartPoint(void *a_pParam);

        virtual void Run() = 0;
        virtual void OnTerminate() = 0;

        void Join(pthread_t a_nID);

        void SetState(THREAD_STATE a_eState);

private:
        /* Disable Copy */
        PThread(const PThread&);
        PThread& operator=(const PThread&);

        /* Attribute */
        pthread_t m_nID;
        char *m_szName;

        THREAD_STATE m_eState;
        THREAD_EXIT_STATE m_eExitType;
};

typedef struct tagPos
{
	void *p;
	struct tagPos *prev;
	struct tagPos *next;

}Pos, *POSITION;

class CMyList
{
public:
	CMyList() {
		m_nCount = 0; 
		m_pHead = NULL;
		m_pTail = NULL;

		//pthread_mutex_init(&m_m, NULL);
	}

	~CMyList() {
		//pthread_mutex_destroy(&m_m);
	};

	int GetCount() { return m_nCount; };
	POSITION GetHeadPosition() { 
		POSITION pos = NULL;

		//pthread_mutex_lock(&m_m);
		pos = m_pHead;
		//pthread_mutex_unlock(&m_m);

		return pos; 
	};

	POSITION GetTailPosition() { 
		POSITION pos = NULL;

		//pthread_mutex_lock(&m_m);
		pos = m_pTail;
		//pthread_mutex_unlock(&m_m);

		return pos;
	};

	void AddHead(void *p) {
		POSITION pp = new Pos;
		pp->p = p;

		//pthread_mutex_lock(&m_m);
		if (m_pHead) {
			pp->next = m_pHead;
			m_pHead->prev = pp;
		}
		m_pHead = pp;
		if (!m_pTail) {
			pp->prev = NULL;
			pp->next = NULL;
			m_pTail = m_pHead;
		}
		//pthread_mutex_unlock(&m_m);

		m_nCount++;
	}

	void AddTail(void *p) {
		POSITION pp = new Pos;
		pp->p = p;
		pp->next = NULL;

		//pthread_mutex_lock(&m_m);
		if (m_pTail) {
			pp->prev = m_pTail;
			m_pTail->next = pp;
			m_pTail = pp;
		}
		if (!m_pHead) {
			pp->prev = NULL;
			pp->next = NULL;
			m_pHead = m_pTail = pp;
		}
		//pthread_mutex_unlock(&m_m);
		m_nCount++;
	};

	void RemoveAt(POSITION &pos) {
		//pthread_mutex_lock(&m_m);
		POSITION p1 = pos->prev;
		POSITION p2 = pos->next;
		if (!p1) { // in case of head
            if (p2) {
                p2->prev = NULL;
                m_pHead = p2;
            } else {
                m_pHead = m_pTail = NULL;
            }
		} else {
			p1->next = p2;
			if (p2) {
				p2->prev = p1;
			} else {
				m_pTail = p1;
			}
		}
		//pthread_mutex_unlock(&m_m);
		m_nCount--;
		delete pos;
	}

	void *RemoveHead() {
		void *p = NULL;

		//pthread_mutex_lock(&m_m);
		p = m_pHead->p;
		POSITION pp = m_pHead;
		if (m_pHead->next) {
			m_pHead = m_pHead->next;
			m_pHead->prev = NULL;
		} else {
			m_pHead = NULL;
			if (m_pTail == pp) {
				m_pTail = NULL;
			}
		}
		//pthread_mutex_unlock(&m_m);

		delete pp;
		m_nCount--;

		return p;
	}

	void RemoveAt(int nIndex) {
		int i;
		POSITION pos = NULL;

		//pthread_mutex_lock(&m_m);
		pos = m_pHead;
		for (i=0; i<nIndex; i++) {
			pos = pos->next;
		}
		//pthread_mutex_unlock(&m_m);

		RemoveAt(pos);
	}

	void RemoveAll() {
		POSITION pos = m_pHead;
		while(pos) {
			POSITION posPrev = pos;
			pos = posPrev->next;
			delete posPrev;
		}
        m_pHead = m_pTail = NULL;
	};

	void *GetNext(POSITION &pos) {
		void *pRet = NULL;

		//pthread_mutex_lock(&m_m);
		pRet = pos->p;
		pos = pos->next;
		//pthread_mutex_unlock(&m_m);

		return pRet;
	};
	void *GetPrev(POSITION &pos) {
		void *pRet = NULL;

		//pthread_mutex_lock(&m_m);
		pRet = pos->p;
		pos = pos->prev;
		//pthread_mutex_unlock(&m_m);

		return pRet;
	};

	void *GetAt(int nIndex) {

		POSITION pos = NULL;
		//pthread_mutex_lock(&m_m);
		pos = m_pHead;
		for (int i=0; i<nIndex; i++) {
			pos = pos->next;
		}
		//pthread_mutex_unlock(&m_m);
		return pos->p;
	};

protected:

	POSITION m_pHead;
	POSITION m_pTail;

	//pthread_mutex_t m_m;

	int m_nCount;

};


#endif // _MISC_H_

