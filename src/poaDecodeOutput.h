#ifndef POA_DECODE_OUTPUT_H
#define POA_DECODE_OUTPUT_H

#include "poaBase.h"

class poaDecodeOutput : public poaBase
{
private:
    /* data */
public:
    /** Construct poaDecodeOutput.
     @param @name The name representing this object. This will be used for logging purposes. 
     */
    poaDecodeOutput(const char *name);
    ~poaDecodeOutput();

    virtual int HandlePaStreamCallback(const void *inputBuffer,
                                       void *outputBuffer,
                                       unsigned long framesPerBuffer,
                                       const PaStreamCallbackTimeInfo *timeInfo,
                                       PaStreamCallbackFlags statusFlags) override;
};

#endif