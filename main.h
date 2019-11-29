#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <libgen.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <semaphore.h>
#include <sys/mman.h>

//C++ lib
#include <chrono>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>

#include "json/json.h"

extern "C" {
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/random_seed.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}


#include "misc.h"

#define PACKET_SIZE 4096

#ifdef DEBUG
#undef DEBUG
#define _d(fmt, args...) { printf(fmt, ## args); }
#else
#undef DEBUG
#define _d(fmt, args...) { printf(fmt, ## args); }
//#define _d(fmt, args...)
#endif

#define MAX_PROFILES			5
#define MAX_AUDIO_TRACKS		2
#define STAT_VIDEO_LOST			0x1000

#define SAFE_DELETE(x) { if (x) delete x; x = NULL; }
#endif