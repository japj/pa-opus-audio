#ifndef POA_DECODE_OUTPUT_H
#define POA_DECODE_OUTPUT_H

#include "poaBase.h"

class poaDecodeOutput : public poaBase
{
private:
    /* data */
    bool isWriteEncodedOpusFrameCalled;

protected:
    virtual PaError HandleOpenDeviceStream() override;

public:
    /** Construct poaDecodeOutput.
     @param @name The name representing this object. This will be used for logging purposes. 
     */
    poaDecodeOutput(const char *name);
    ~poaDecodeOutput();

    bool writeEncodedOpusFrame(/*int &sequence_number, */ void *data, int data_length);

    virtual int _HandlePaStreamCallback(const void *inputBuffer,
                                        void *outputBuffer,
                                        unsigned long framesPerBuffer,
                                        const PaStreamCallbackTimeInfo *timeInfo,
                                        PaStreamCallbackFlags statusFlags) override;
};

#endif