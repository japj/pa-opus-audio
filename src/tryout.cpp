#include "poaDecodeOutput.h"
#include "poaEncodeInput.h"

#define LOGERR(r, call)                                              \
    {                                                                \
        if (r < 0)                                                   \
        {                                                            \
            printf("ERROR: '%s' in %s\n", Pa_GetErrorText(r), call); \
        }                                                            \
    }

int tryout()
{
    poaEncodeInput input("input");
    poaDecodeOutput output("output");

    input.log("testing %d\n", 1);
    output.log("testing %s\n", "foo");

    PaError err;

    err = input.OpenInputDeviceStream(1);
    LOGERR(err, "input.OpenInputDeviceStream");

    err = output.OpenOutputDeviceStream();
    LOGERR(err, "output.OpenOutputDeviceStream");

    return 0;
}