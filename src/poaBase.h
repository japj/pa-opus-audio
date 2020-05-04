#ifndef PAO_BASE_H
#define PAO_BASE_H

#include <string>
#include "portaudio.h"

/* our custom way of default device selection */
#define poaDefaultDevice ((PaDeviceIndex)-2)

#define PaLOGERR(r, call)        \
    {                            \
        if (r < 0)               \
        {                        \
            logPaError(r, call); \
        }                        \
    }

/**
 * Device Data structure holding all related device settings required for setting up PortAudio stream
 */
typedef struct poaDeviceData
{
    PaStreamParameters streamParams;
    double sampleRate;
    PaStreamFlags streamFlags;
    unsigned long framesPerBuffer;
    int sampleSize; // calculated based on sampleFormat
} poaDeviceData;

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
    static int paStaticStreamCallback(const void *inputBuffer,
                                      void *outputBuffer,
                                      unsigned long framesPerBuffer,
                                      const PaStreamCallbackTimeInfo *timeInfo,
                                      PaStreamCallbackFlags statusFlags,
                                      void *userData);

    /** Functions of type PaStreamFinishedCallback are implemented by PortAudio 
     clients. They can be registered with a stream using the Pa_SetStreamFinishedCallback
    function. Once registered they are called when the stream becomes inactive
    (ie once a call to Pa_StopStream() will not block).
    A stream will become inactive after the stream callback returns non-zero,
    or when Pa_StopStream or Pa_AbortStream is called. For a stream providing audio
    output, if the stream callback returns paComplete, or Pa_StopStream() is called,
    the stream finished callback will not be called until all generated sample data
    has been played.
    */
    static void paStaticStreamFinishedCallback(void *userdata);

    // TODO: check if mutex on access is needed?
    bool isCallbackRunning;

    void setupDefaultDeviceData(poaDeviceData *data);
    /* some deviceData is calculated, whenever you change certain settings call this to recalculate
     */
    void recalculateDeviceData(poaDeviceData *data);

    /** Initializes the device for streaming audio input/output
     @param inputDevice specifies the input device to be used, paNoDevice in case no input device should be used
                  poaDefaultDevice in case the system default input device should be used
     @param outputDevice  specifies the output device to be used, paNoDevice in case no output device should be used
                  poaDefaultDevice in case the system default input device should be used
     */
    PaError OpenDeviceStream(PaDeviceIndex inputDevice = paNoDevice, PaDeviceIndex outputDevice = paNoDevice);

protected:
    // access from Decode/Output needed
    poaDeviceData inputData;
    poaDeviceData outputData;

public:
    /** Construct poaBase.
     @param @name The name representing this object. This will be used for logging purposes. 
     */
    poaBase(const char *name);
    ~poaBase();

    void log(const char *format, ...);
    void logPaError(PaError err, const char *format, ...);

    /* Determine if the stream callback is currently running
     */
    bool IsCallbackRunning();

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

    /* called by low level paStaticStreamCallback , this needs to be public else it can't access it
       and it needs to be overridden by actual implementation 
    */
    virtual int HandlePaStreamCallback(const void *inputBuffer,
                                       void *outputBuffer,
                                       unsigned long framesPerBuffer,
                                       const PaStreamCallbackTimeInfo *timeInfo,
                                       PaStreamCallbackFlags statusFlags) = 0;

    /** called by low level paStaticStreamFinishedCallback, this needs to be public elase can't access it
     * end it can be overridden
    */
    virtual void HandlePaStreamFinished();
};

#endif