#include "poaBase.h"

poaBase::poaBase(const char *name) : name(name)
{
}

poaBase::~poaBase()
{
}

void poaBase::log(const char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    printf("%s: %s", name.c_str(), buffer);
}