#ifndef _PA_COMMON_H
#define _PA_COMMON_H

#ifdef _WIN32
//TODO: check if aligned alloc is really needed?
#define ALLIGNEDMALLOC(x) _aligned_malloc(x, 32)
#define ALLIGNEDFREE(x) _aligned_free
#include <chrono>
#include <thread>
//TODO: replace usleep with this everywhere
#define usleep(x) std::this_thread::sleep_for(std::chrono::microseconds(x))
#endif

#ifdef __APPLE__
#include <unistd.h>
#define ALLIGNEDMALLOC(x) valloc(x)
#define ALLIGNEDFREE(x) free(x)
#endif

#ifdef __linux__
// TODO check cross platform alternative
// TODO: currently linux ALLIGNEDMALLOC is actually not aligned
#include <stdlib.h>
#define ALLIGNEDMALLOC(x) malloc(x)
#define ALLIGNEDFREE(x) free(x)
#endif

#include <opus/opus.h>

#include "portaudio.h"
#include "pa_ringbuffer.h"

#define PaCHK(call, r)        \
	{                         \
		if (r < 0)            \
		{                     \
			paerror(call, r); \
			return -1;        \
		}                     \
	}

int setupPa();
int terminatePa();
int protoring();
void paerror(const char *msg, int r);
void log_pa_stream_info(PaStream *stream, PaStreamParameters *params);

/* our custom way of default device selection */
#define paDefaultDevice ((PaDeviceIndex)-2)

int calcSizeUpPow2(unsigned int len);
/*v--;
v |= v >> 1;
v |= v >> 2;
v |= v >> 4;
v |= v >> 8;
v |= v >> 16;
v++;
*/

#endif