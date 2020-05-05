#ifndef ENCODE_WORKER
#define ENCODE_WORKER

#include <napi.h>

#include "paEncodeInStream.h"

class EncodeWorker : public Napi::AsyncWorker
{
public:
    EncodeWorker(Napi::Function &callback, paEncodeInStream *output);

    void Execute() override;

    void OnOK() override;

    void Destroy() override;

private:
    uint8_t *data;
    size_t encoded_size;
    paEncodeInStream *input;
};

#endif