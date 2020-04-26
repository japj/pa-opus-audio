#pragma once

#include <napi.h>

class Rehearse20 : public Napi::ObjectWrap<Rehearse20>
{
public:
    Rehearse20(const Napi::CallbackInfo&);
    Napi::Value Greet(const Napi::CallbackInfo&);

    static Napi::Function GetClass(Napi::Env);

    //PortAudio
    Napi::Value Detect(const Napi::CallbackInfo& info);
    Napi::Value Protoring(const Napi::CallbackInfo& info);

private:
    std::string _greeterName;
};
