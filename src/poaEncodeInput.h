#ifndef POA_ENCODE_INPUT_H
#define POA_ENCODE_INPUT_H

#include "poaBase.h"

class poaEncodeInput : public poaBase
{
private:

protected:
    virtual PaError HandleOpenDeviceStream() override;

public:
    /** Construct poaEncodeInput.
     @param @name The name representing this object. This will be used for logging purposes. 
     */
    poaEncodeInput(const char *name);
    ~poaEncodeInput();

    virtual int _HandlePaStreamCallback(const void *inputBuffer,
                                        void *outputBuffer,
                                        unsigned long framesPerBuffer,
                                        const PaStreamCallbackTimeInfo *timeInfo,
                                        PaStreamCallbackFlags statusFlags) override;
};

#endif