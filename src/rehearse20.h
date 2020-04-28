#pragma once

#include <napi.h>
#include "paDecodeOutStream.h"
#include "paEncodeInStream.h"
#include "paStreamCommon.h"

class Rehearse20 : public Napi::ObjectWrap<Rehearse20>
{
public:
    Rehearse20(const Napi::CallbackInfo&);
    Napi::Value Greet(const Napi::CallbackInfo&);

    static Napi::Function GetClass(Napi::Env);

    //PortAudio
    Napi::Value Detect(const Napi::CallbackInfo& info);
    Napi::Value Protoring(const Napi::CallbackInfo& info);

    Napi::Value OutputInitAndStartStream(const Napi::CallbackInfo& info);
    Napi::Value DecodeDataIntoPlayback(const Napi::CallbackInfo& info);

    Napi::Value InputInitAndStartStream(const Napi::CallbackInfo& info);
    Napi::Value EncodeRecordingIntoData(const Napi::CallbackInfo& info);

private:
    std::string _greeterName;
    paDecodeOutStream output;
    paEncodeInStream input;
};
