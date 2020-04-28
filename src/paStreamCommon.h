#ifndef _PA_COMMON_H
#define _PA_COMMON_H

#ifdef _WIN32
//TODO: check if aligned alloc is really needed?
#define ALLIGNEDMALLOC(x) _aligned_malloc(x, 32)
#define ALLIGNEDFREE(x) _aligned_free
#include <chrono>
#include <thread>
#define usleep(x) std::this_thread::sleep_for(std::chrono::microseconds(x))
#else
#include <unistd.h>
#define ALLIGNEDMALLOC(x) valloc(x)
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
int protoring();
void paerror(const char *msg, int r);
void log_pa_stream_info(PaStream *stream, PaStreamParameters *params);

/* our custom way of default device selection */
#define paDefaultDevice ((PaDeviceIndex)-2)

#endif