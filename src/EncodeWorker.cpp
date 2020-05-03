#include "EncodeWorker.h"

EncodeWorker::EncodeWorker(Napi::Function &callback, paEncodeInStream *input) : Napi::AsyncWorker(callback), input(input), data(0), encoded_size(0)
{
}

void EncodeWorker::Execute()
{
    int encodeBufferSize = input->GetUncompressedBufferSizeBytes();
    this->data = (uint8_t *)malloc(encodeBufferSize);

    this->encoded_size = input->EncodeRecordingIntoData(this->data, encodeBufferSize);

    int opusFullFramesReadAvailable = input->GetOpusFullFramesReadAvailable();
    if (opusFullFramesReadAvailable > 0)
    {
        printf("EncodeWorker::Execute could do (%d) more opus encodings after already done encoding\n", opusFullFramesReadAvailable);
    }
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