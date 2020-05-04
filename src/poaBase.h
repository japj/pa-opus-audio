#ifndef PAO_BASE_H
#define PAO_BASE_H

#include <string>

class poaBase
{
private:
    /* data */
    //
    std::string name; 
public:
    poaBase(const char *name);
    ~poaBase();

    void log(const char * format, ...);
};

#endif