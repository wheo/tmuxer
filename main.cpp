#include "main.h"
#include "core.h"

#include <sys/time.h>
#include <sys/resource.h>

bool exit_flag_main = false;

pthread_mutex_t sleepMutex;
pthread_cond_t  sleepCond;

void sigfunc(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        exit_flag_main = true;
    }
    pthread_cond_signal(&sleepCond);
}

static void log_callback(void *ptr, int level, const char *fmt, va_list vl) {
	char szMsg[512] = { 0};
	vsprintf(szMsg, fmt, vl);

	_d("%s\n", szMsg);
}

int main(int argc, char *argv[])
{
	struct rlimit rlim;
	signal(SIGINT, sigfunc);
    signal(SIGTERM, sigfunc);
    signal(SIGHUP, sigfunc);

#if 0
    getrlimit(RLIMIT_STACK, &rlim);
    rlim.rlim_cur = (4096*1024*10);
    rlim.rlim_max = (4096*1024*10);
    setrlimit(RLIMIT_STACK, &rlim);
#endif

    //setting.json check
    ifstream ifs("./setting.json");

    if ( !ifs.is_open()) {
        _d("\n ******************************************* \n setting.json file is not found\n Put in setting.json file in your directory\n ******************************************* \n\n");
        exit(0);
    } else {
        //check done
        ifs.close();
    }

	//void av_register_all()’ is deprecated [-Wdeprecated-declarations]
    //av_register_all();
    //avfilter_register_all();
	_d("[MAIN] Started...\n");
    CCore *core = new CCore();
    core->Create();

	while(!exit_flag_main) {
        pthread_mutex_lock(&sleepMutex);
        pthread_cond_wait(&sleepCond, &sleepMutex);
        pthread_mutex_unlock(&sleepMutex);
    }
	_d("[MAIN] Exiting...\n");

    SAFE_DELETE(core);

	_d("[MAIN] Exited\n");
	return 0;
}
