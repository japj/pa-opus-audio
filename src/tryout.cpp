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

    err = input.OpenInputDeviceStream();
    LOGERR(err, "input.OpenInputDeviceStream");

    err = output.OpenOutputDeviceStream();
    LOGERR(err, "output.OpenOutputDeviceStream");

    printf("input callback running: %d\n", input.IsCallbackRunning());
    printf("output callback running: %d\n", input.IsCallbackRunning());

    printf("StartStream\n");
    err = input.StartStream();
    LOGERR(err, "input.StartStream");

    err = output.StartStream();
    LOGERR(err, "output.StartStream");

    Pa_Sleep(1000);

    printf("input callback running: %d\n", input.IsCallbackRunning());
    printf("output callback running: %d\n", input.IsCallbackRunning());

    printf("StopStream\n");
    err = input.StopStream();
    LOGERR(err, "input.StopStream");

    err = output.StopStream();
    LOGERR(err, "output.StopStream");

    printf("input callback running: %d\n", input.IsCallbackRunning());
    printf("output callback running: %d\n", input.IsCallbackRunning());

    printf("CloseStream\n");
    err = input.CloseStream();
    LOGERR(err, "input.CloseStream");

    err = output.CloseStream();
    LOGERR(err, "output.CloseStream");

    printf("input callback running: %d\n", input.IsCallbackRunning());
    printf("output callback running: %d\n", input.IsCallbackRunning());

    Pa_Sleep(1000);

    printf("input callback running: %d\n", input.IsCallbackRunning());
    printf("output callback running: %d\n", input.IsCallbackRunning());

    printf("Exiting...\n");
    return 0;
}