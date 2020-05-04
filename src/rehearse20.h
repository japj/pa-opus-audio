#pragma once

#include <napi.h>
#include "paDecodeOutStream.h"
#include "paEncodeInStream.h"
#include "paStreamCommon.h"

class Rehearse20 : public Napi::ObjectWrap<Rehearse20>
{
public:
    Rehearse20(const Napi::CallbackInfo &);
    Napi::Value Greet(const Napi::CallbackInfo &);

    static Napi::Function GetClass(Napi::Env);

    //PortAudio
    Napi::Value Tryout(const Napi::CallbackInfo &info);
    Napi::Value Detect(const Napi::CallbackInfo &info);
    Napi::Value Protoring(const Napi::CallbackInfo &info);

    Napi::Value OutputInitAndStartStream(const Napi::CallbackInfo &info);
    Napi::Value DecodeDataIntoPlayback(const Napi::CallbackInfo &info);
    Napi::Value OutputStopStream(const Napi::CallbackInfo &info);

    Napi::Value InputInitAndStartStream(const Napi::CallbackInfo &info);
    Napi::Value InputStopStream(const Napi::CallbackInfo &info);

    Napi::Value SetEncodedFrameAvailableCallBack(const Napi::CallbackInfo &info);

    // internal callback for NAPI so we can call Js Function within right context
    static void JsThreadHandleEncodeInStreamCallback(Napi::Env env, Napi::Function jsCallback, void *userData);
    void handleEncodeInStreamCallback(Napi::Env env, Napi::Function jsCallback);

private:
    std::string _greeterName;
    paDecodeOutStream output;
    paEncodeInStream input;
    int encodeBufferSize;
    uint8_t *encodeBuffer;

    bool tsfnSet;
    Napi::ThreadSafeFunction tsfn;

    void handleEncodeInStreamCallback();
    static void paEncodeInStreamOpusFrameAvailableCallback(void *userData);
};
