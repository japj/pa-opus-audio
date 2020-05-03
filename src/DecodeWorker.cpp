#include "DecodeWorker.h"

DecodeWorker::DecodeWorker(Napi::Env env, Napi::Buffer<uint8_t> buffer, paDecodeOutStream *output)
    : Napi::AsyncWorker(env), output(output)
{
    length = buffer.Length();
    data = (uint8_t *)malloc(length);
    memcpy(data, buffer.Data(), length);
}

void DecodeWorker::Execute()
{
    if (data)
    {
        int result = output->DecodeDataIntoPlayback(data, length);
    }
    // TODO error handling for this?
}