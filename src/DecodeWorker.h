#ifndef DECODE_WORKER
#define DECODE_WORKER

#include <napi.h>

#include "paDecodeOutStream.h"

class DecodeWorker : public Napi::AsyncWorker
{
public:
    DecodeWorker(Napi::Env env, Napi::Buffer<uint8_t> buffer, paDecodeOutStream *output);

    void Execute() override;

private:
    uint8_t *data;
    size_t length;
    paDecodeOutStream *output;
};

#endif