#include "rehearse20.h"

using namespace Napi;

Rehearse20::Rehearse20(const Napi::CallbackInfo &info) : ObjectWrap(info)
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

    int result;

    result = setupPa();
    if (result == 0)
    {
        result = protoring();
    }

    return Napi::Number::New(env, result);
}

Napi::Value Rehearse20::OutputInitAndStartStream(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    int result = setupPa();
    if (result == 0)
    {
        result = output.InitForDevice();
        if (result == 0)
        {
            result = output.StartStream();
        }
    }

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

    int result = output.DecodeDataIntoPlayback(buffer.Data(), (int)buffer.Length());

    return Napi::Number::New(env, result);
}

Napi::Value Rehearse20::InputInitAndStartStream(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    int result = setupPa();
    if (result == 0)
    {
        result = input.InitForDevice();

        encodeBufferSize = input.GetUncompressedBufferSizeBytes();
        encodeBuffer = (uint8_t *)ALLIGNEDMALLOC(encodeBufferSize);

        input.setUserCallbackOpusFrameAvailable(paEncodeInStreamOpusFrameAvailableCallback, this);

        if (result == 0)
        {
            result = input.StartStream();
        }
    }

    return Napi::Number::New(env, 0);
}

Napi::Value Rehearse20::EncodeRecordingIntoData(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    int encodedPacketSize = input.EncodeRecordingIntoData(this->encodeBuffer, this->encodeBufferSize);
    if (encodedPacketSize > 0)
    {
        // TODO: busy looping, needs API design based on notification/callbacks

        Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, this->encodeBuffer, encodedPacketSize);
        return buffer;
    }
    else
    {
        // return NULL object
        return env.Null();
    }
}

// static callback for NAPI
void Rehearse20::JsThreadHandleEncodeInStreamCallback(Napi::Env env, Napi::Function jsCallback, void *value)
{
    Rehearse20 *p = (Rehearse20 *)value;

    p->handleEncodeInStreamCallback(env, jsCallback);
}

void Rehearse20::handleEncodeInStreamCallback(Napi::Env env, Napi::Function jsCallback)
{
    int encodedPacketSize = input.EncodeRecordingIntoData(this->encodeBuffer, this->encodeBufferSize);
    if (encodedPacketSize > 0)
    {
        Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::Copy(env, this->encodeBuffer, encodedPacketSize);
        jsCallback.Call({buffer});
    }
    else
    {
        // return NULL object
        jsCallback.Call({env.Null()});
    }
}

// trigger the async execution of the ThreadSafeFunction
void Rehearse20::handleEncodeInStreamCallback()
{

    if (!tsfnSet)
    {
        printf("handleEncodeInStreamCallback: tsfn not set!\n"); // TODO: better handling if user actually registered a callback?
        return;
    }

    napi_status status = tsfn.NonBlockingCall(this, JsThreadHandleEncodeInStreamCallback);
    if (status != napi_ok)
    {
        printf("handleEncodeInStreamCallback: napi-error %d", status); //TODO better handling
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

Napi::Function Rehearse20::GetClass(Napi::Env env)
{
    return DefineClass(env, "Rehearse20", {
                                              Rehearse20::InstanceMethod("greet", &Rehearse20::Greet),
                                              Rehearse20::InstanceMethod("detect", &Rehearse20::Detect),
                                              Rehearse20::InstanceMethod("protoring", &Rehearse20::Protoring),
                                              Rehearse20::InstanceMethod("OutputInitAndStartStream", &Rehearse20::OutputInitAndStartStream),
                                              Rehearse20::InstanceMethod("DecodeDataIntoPlayback", &Rehearse20::DecodeDataIntoPlayback),
                                              Rehearse20::InstanceMethod("InputInitAndStartStream", &Rehearse20::InputInitAndStartStream),
                                              //Rehearse20::InstanceMethod("EncodeRecordingIntoData", &Rehearse20::EncodeRecordingIntoData),
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
