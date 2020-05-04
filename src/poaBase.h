#ifndef PAO_BASE_H
#define PAO_BASE_H

#include <string>
#include "portaudio.h"

/* our custom way of default device selection */
#define poaDefaultDevice ((PaDeviceIndex)-2)

class poaBase
{
private:
    /* data */
    //
    std::string name;

    /** current stream, NULL if no stream was opened yet */
    PaStream *stream;

    /* This routine will be called by the PortAudio engine when audio is needed.
    ** It may called at interrupt level on some machines so don't do anything
    ** that could mess up the system like calling malloc() or free().
    */
    static int paStaticCallback(const void *inputBuffer,
                                void *outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo *timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void *userData);

    /** current selected inputDevice */
    PaDeviceIndex inputDevice;
    /** current selected outputDevice */
    PaDeviceIndex outputDevice;

    /** Initializes the device for streaming audio input/output
     @param inputDevice specifies the input device to be used, paNoDevice in case no input device should be used
                  poaDefaultDevice in case the system default input device should be used
     @param outputDevice  specifies the output device to be used, paNoDevice in case no output device should be used
                  poaDefaultDevice in case the system default input device should be used
     */
    PaError OpenDeviceStream(PaDeviceIndex inputDevice = paNoDevice, PaDeviceIndex outputDevice = paNoDevice);

public:
    /** Construct poaBase.
     @param @name The name representing this object. This will be used for logging purposes. 
     */
    poaBase(const char *name);
    ~poaBase();

    void log(const char *format, ...);

    /** Initializes the device for streaming audio input
        @param inputDevice specifies the input device to be used, paNoDevice in case no input device should be used
                  poaDefaultDevice in case the system default input device should be used
     */
    PaError OpenInputDeviceStream(PaDeviceIndex inputDevice = poaDefaultDevice);

    /** Initializes the device for streaming audio output
        @param inputDevice specifies the input device to be used, paNoDevice in case no input device should be used
                  poaDefaultDevice in case the system default input device should be used
     */
    PaError OpenOutputDeviceStream(PaDeviceIndex output = poaDefaultDevice);

    /**
     * Generic PortAudio Related functions
     */

    /** Commences audio processing.
    */
    PaError StartStream();

    /** Determine whether the stream is active.
     */
    PaError IsStreamActive();

    /** Terminates audio processing. It waits until all pending
     audio buffers have been played before it returns.
    */
    PaError StopStream();

    /** Closes an audio stream. If the audio stream is active it
     discards any pending buffers as if Pa_AbortStream() had been called.
    */
    PaError CloseStream();

    /* called by low level callback , this needs to be public else it can't access it
       and it needs to be overridden by actual implementation 
    */
    virtual int HandlePaCallback(const void *inputBuffer,
                                 void *outputBuffer,
                                 unsigned long framesPerBuffer,
                                 const PaStreamCallbackTimeInfo *timeInfo,
                                 PaStreamCallbackFlags statusFlags) = 0;
};

#endif