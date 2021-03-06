#include <string.h>
#include "rehearse20.h"
#if USE_OLD_ENCODE_DECODE_STREAM
#include "DecodeWorker.h"
#include "EncodeWorker.h"
#else
int protoring();
#endif
#include "tryout.h"

using namespace Napi;

Rehearse20::Rehearse20(const Napi::CallbackInfo &info) : ObjectWrap(info),
                                                         input("input"),
                                                         output("output"),
                                                         outputSequenceNumber(0)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1)
    {
        Napi::TypeError::New(env, "Wrong number of arguments")
            .ThrowAsJavaScriptException();
        return;
    }

    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "You need to name yourself")
            .ThrowAsJavaScriptException();
        return;
    }

    this->_greeterName = info[0].As<Napi::String>().Utf8Value();
    std::string inString = this->_greeterName + "input";
    std::string outString = this->_greeterName + "output";

    this->input.setName(inString.c_str());
    this->output.setName(outString.c_str());

    tsfnSet = false;
}

Napi::Value Rehearse20::Greet(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1)
    {
        Napi::TypeError::New(env, "Wrong number of arguments")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "You need to introduce yourself to greet")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::String name = info[0].As<Napi::String>();

    printf("Hello %s\n", name.Utf8Value().c_str());
    printf("I am %s\n", this->_greeterName.c_str());

    return Napi::String::New(env, this->_greeterName);
}

extern "C" int detect();

Napi::Value Rehearse20::Detect(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    int result = detect();

    return Napi::Number::New(env, result);
}

Napi::Value Rehearse20::Protoring(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    int result = protoring();

    return Napi::Number::New(env, result);
}

Napi::Value Rehearse20::OutputInitAndStartStream(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    int result = 0;
#if USE_OLD_ENCODE_DECODE_STREAM
    result = output.InitForDevice();
    if (result == 0)
    {
        result = output.StartStream();
    }
#else
    result = output.OpenOutputDeviceStream();
    if (result == 0)
    {
        result = output.StartStream();
    }
#endif

    return Napi::Number::New(env, result);
}

Napi::Value Rehearse20::DecodeDataIntoPlayback(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 1)
    {
        Napi::TypeError::New(env, "Wrong number of arguments")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsBuffer())
    {
        Napi::TypeError::New(env, "Need data Buffer as argument 1")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    Buffer<uint8_t> buffer = info[0].As<Buffer<uint8_t>>();

#if USE_OLD_ENCODE_DECODE_STREAM
    // run the actual decoding in a seperate worker
    // TODO: how to ensure multiple DecodeWorkers don't run into each other (or order gets confused)?
    DecodeWorker *wk = new DecodeWorker(env, buffer, &output);
    wk->Queue();
#else
    // TODO, split encoded data and sequencenumber for js part in orde to send across network
    poaCallbackTransferData tData;
    memset(&tData, 0, sizeof(tData));
    tData.dataLength = buffer.Length();
    memcpy(tData.data, buffer.Data(), tData.dataLength);
    tData.sequenceNumber = outputSequenceNumber++; // FAKE sequencenumber, since JS api doesn't have it
    output.writeEncodedOpusFrame(&tData);
    //output.writeEncodedOpusFrame((poaCallbackTransferData *)buffer.Data());
#endif

    return Napi::Number::New(env, 0);
}

Napi::Value Rehearse20::InputInitAndStartStream(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    int result = 0;
#if USE_OLD_ENCODE_DECODE_STREAM
    result = input.InitForDevice();

    encodeBufferSize = input.GetUncompressedBufferSizeBytes();
    encodeBuffer = (uint8_t *)ALLIGNEDMALLOC(encodeBufferSize);

    input.setUserCallbackOpusFrameAvailable(paEncodeInStreamOpusFrameAvailableCallback, this);

    if (result == 0)
    {
        result = input.StartStream();
    }
#else
    input.registerOpusFrameAvailableCb(paEncodeInStreamOpusFrameAvailableCallback, this);
    result = input.OpenInputDeviceStream();
    if (result == 0)
    {
        result = input.StartStream();
    }
#endif

    return Napi::Number::New(env, result);
}

// static callback for NAPI
void Rehearse20::JsThreadHandleEncodeInStreamCallback(Napi::Env env, Napi::Function jsCallback, void *value)
{
    Rehearse20 *p = (Rehearse20 *)value;

    p->handleEncodeInStreamCallback(env, jsCallback);
}

void Rehearse20::handleEncodeInStreamCallback(Napi::Env env, Napi::Function jsCallback)
{
    // run the actual encoding in a seperate worker
    // TODO: how to ensure multiple EncodeWorkers don't run into each other (or order gets confused)?
#if USE_OLD_ENCODE_DECODE_STREAM
    EncodeWorker *wk = new EncodeWorker(jsCallback, &this->input);
    wk->Queue();
#else
    // TODO
    poaCallbackTransferData tData;
    bool read = input.readEncodedOpusFrame(&tData);
    if (read)
    {
        // TODO: sequenceNumber
        //Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(Env(), (uint8_t *)tData->data, sizeof(poaCallbackTransferData));
        Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(Env(), (uint8_t *)tData.data, tData.dataLength);
        jsCallback.Call({buffer});
    }
    else
    {
        printf("readEncodedOpusFrame failed?? \n");
        jsCallback.Call({Env().Null()});
    }
#endif
}

// trigger the async execution of the ThreadSafeFunction
void Rehearse20::handleEncodeInStreamCallback()
{
    napi_status status;

    if (!tsfnSet)
    {
        printf("handleEncodeInStreamCallback: tsfn not set!\n"); // TODO: better handling if user actually registered a callback?
        return;
    }

    // acquire tsfn lock
    status = tsfn.Acquire();
    if (status != napi_ok)
    {
        printf("handleEncodeInStreamCallback:tsfn.Acquire napi-error %d", status); //TODO better handling
    }

    status = tsfn.NonBlockingCall(this, JsThreadHandleEncodeInStreamCallback);
    if (status != napi_ok)
    {
        printf("handleEncodeInStreamCallback: napi-error %d", status); //TODO better handling
    }

    status = tsfn.Release();
    if (status != napi_ok)
    {
        printf("handleEncodeInStreamCallback:tsfn.Release napi-error %d", status); //TODO better handling
    }
}

// static callback to register in paEncodeInStream
void Rehearse20::paEncodeInStreamOpusFrameAvailableCallback(void *userData)
{
    Rehearse20 *p = (Rehearse20 *)userData;
    p->handleEncodeInStreamCallback();
}

Napi::Value Rehearse20::SetEncodedFrameAvailableCallBack(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (tsfnSet)
    {
        throw TypeError::New(env, "Callback already set");
    }

    if (info.Length() != 1)
    {
        throw TypeError::New(env, "Expected 1 argument");
    }
    else if (!info[0].IsFunction())
    {
        throw TypeError::New(env, "Expected first arg to be a function");
    }

    tsfn = Napi::ThreadSafeFunction::New(
        env,
        info[0].As<Napi::Function>(), // js function to call asynchronous
        "EncodedFrameAvailableCallBack",
        0, // unlimited queue
        2  // 2 threads, since we don't call it from main (and that is included in the count?)
    );
    tsfnSet = true;

    return Napi::Number::New(env, 0);
}

Napi::Value Rehearse20::OutputStopStream(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    PaError err = output.StopStream();
    if (err)
    {
        std::string msg = "OutputStopStream error:" + std::to_string(err);
        throw TypeError::New(env, msg.c_str());
    }
    err = output.CloseStream();
    if (err)
    {
        std::string msg = "OutputCloseStream error" + std::to_string(err);
        throw TypeError::New(env, msg.c_str());
    }

    return Napi::Number::New(env, err);
}

Napi::Value Rehearse20::InputStopStream(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    PaError err = input.StopStream();
    if (err)
    {
        std::string msg = "InputStopStream error: " + std::to_string(err);
        throw TypeError::New(env, msg.c_str());
    }
    err = input.CloseStream();
    if (err)
    {
        std::string msg = "InputCloseStream error:" + std::to_string(err);
        throw TypeError::New(env, msg.c_str());
    }

    return Napi::Number::New(env, err);
}

Napi::Value Rehearse20::Tryout(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    int result = tryout();

    return Napi::Number::New(env, result);
}

Napi::Function Rehearse20::GetClass(Napi::Env env)
{
    return DefineClass(env, "Rehearse20", {
                                              Rehearse20::InstanceMethod("greet", &Rehearse20::Greet),
                                              Rehearse20::InstanceMethod("tryout", &Rehearse20::Tryout),
                                              Rehearse20::InstanceMethod("detect", &Rehearse20::Detect),
                                              Rehearse20::InstanceMethod("protoring", &Rehearse20::Protoring),
                                              Rehearse20::InstanceMethod("OutputInitAndStartStream", &Rehearse20::OutputInitAndStartStream),
                                              Rehearse20::InstanceMethod("DecodeDataIntoPlayback", &Rehearse20::DecodeDataIntoPlayback),
                                              Rehearse20::InstanceMethod("InputInitAndStartStream", &Rehearse20::InputInitAndStartStream),
                                              Rehearse20::InstanceMethod("OutputStopStream", &Rehearse20::OutputStopStream),
                                              Rehearse20::InstanceMethod("InputStopStream", &Rehearse20::InputStopStream),
                                              Rehearse20::InstanceMethod("SetEncodedFrameAvailableCallBack", &Rehearse20::SetEncodedFrameAvailableCallBack),
                                          });
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    Napi::String name = Napi::String::New(env, "Rehearse20");
    exports.Set(name, Rehearse20::GetClass(env));
    return exports;
}

NODE_API_MODULE(addon, Init)
