#include "EncodeWorker.h"

EncodeWorker::EncodeWorker(Napi::Function &callback, paEncodeInStream *input) : Napi::AsyncWorker(callback), input(input), data(0), encoded_size(0)
{
}

void EncodeWorker::Execute()
{
    int encodeBufferSize = input->GetUncompressedBufferSizeBytes();
    this->data = (uint8_t *)malloc(encodeBufferSize);

    this->encoded_size = input->EncodeRecordingIntoData(this->data, encodeBufferSize);
}

void EncodeWorker::OnOK()
{
    if (encoded_size > 0)
    {
        Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(Env(), this->data, encoded_size);

        Callback().Call({buffer});
    }
    else
    {
        Callback().Call({Env().Null()});
    }
}

void EncodeWorker::Destroy()
{
    if (this->data)
    {
        free(this->data);
        this->data = NULL;
        this->encoded_size = 0;
    }
}