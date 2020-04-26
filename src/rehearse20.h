#pragma once

#include <napi.h>

class Rehearse20 : public Napi::ObjectWrap<Rehearse20>
{
public:
    Rehearse20(const Napi::CallbackInfo&);
    Napi::Value Greet(const Napi::CallbackInfo&);

    static Napi::Function GetClass(Napi::Env);

private:
    std::string _greeterName;
};
