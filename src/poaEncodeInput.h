#ifndef POA_ENCODE_INPUT_H
#define POA_ENCODE_INPUT_H

#include "poaBase.h"

typedef void paEncodeInputOpusFrameAvailableCb(void *userData);

class poaEncodeInput : public poaBase
{
private:
    paEncodeInputOpusFrameAvailableCb *userCallbackOpusFrameAvailableCb;
    void *userCallbackOpusFrameAvailableData;

protected:
    virtual PaError HandleOpenDeviceStream() override;

public:
    /** Construct poaEncodeInput.
     @param @name The name representing this object. This will be used for logging purposes. 
     */
    poaEncodeInput(const char *name);
    ~poaEncodeInput();

    void registerOpusFrameAvailableCb(paEncodeInputOpusFrameAvailableCb cb, void *userData);

    int readEncodedOpusFrame(/*int &sequence_number,*/ void *buffer, int buffer_size);

    virtual int _HandlePaStreamCallback(const void *inputBuffer,
                                        void *outputBuffer,
                                        unsigned long framesPerBuffer,
                                        const PaStreamCallbackTimeInfo *timeInfo,
                                        PaStreamCallbackFlags statusFlags) override;
};

#endif