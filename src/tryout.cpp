#include "poaDecodeOutput.h"
#include "poaEncodeInput.h"

#define LOGERR(r, call)                                              \
    {                                                                \
        if (r < 0)                                                   \
        {                                                            \
            printf("ERROR: '%s' in %s\n", Pa_GetErrorText(r), call); \
        }                                                            \
    }

#define START_INPUT 1
#define START_OUTPUT 1

int tryout()
{
#if START_INPUT
    poaEncodeInput input("input");
    input.log("testing %d\n", 1);
#endif
#if START_OUTPUT
    poaDecodeOutput output("output");
    output.log("testing %s\n", "foo");
#endif

    PaError err;

#if START_INPUT
    err = input.OpenInputDeviceStream();
    LOGERR(err, "input.OpenInputDeviceStream");
#endif

#if START_OUTPUT
    err = output.OpenOutputDeviceStream();
    LOGERR(err, "output.OpenOutputDeviceStream");
#endif

#if START_INPUT
    printf("input callback running: %d\n", input.IsCallbackRunning());
#endif
#if START_OUTPUT
    printf("output callback running: %d\n", output.IsCallbackRunning());
#endif

    printf("StartStream\n");
#if START_INPUT
    err = input.StartStream();
    LOGERR(err, "input.StartStream");
#endif
#if START_OUTPUT
    err = output.StartStream();
    LOGERR(err, "output.StartStream");
#endif

    Pa_Sleep(1000);

#if START_INPUT
    printf("input callback running: %d\n", input.IsCallbackRunning());
#endif
#if START_OUTPUT
    printf("output callback running: %d\n", output.IsCallbackRunning());
#endif

    printf("StopStream\n");
#if START_INPUT
    err = input.StopStream();
    LOGERR(err, "input.StopStream");
#endif
#if START_OUTPUT
    err = output.StopStream();
    LOGERR(err, "output.StopStream");
#endif

#if START_INPUT
    printf("input callback running: %d\n", input.IsCallbackRunning());
#endif
#if START_OUTPUT
    printf("output callback running: %d\n", output.IsCallbackRunning());
#endif

    printf("CloseStream\n");
#if START_INPUT
    err = input.CloseStream();
    LOGERR(err, "input.CloseStream");
#endif
#if START_OUTPUT
    err = output.CloseStream();
    LOGERR(err, "output.CloseStream");
#endif

#if START_INPUT
    printf("input callback running: %d\n", input.IsCallbackRunning());
#endif
#if START_OUTPUT
    printf("output callback running: %d\n", output.IsCallbackRunning());
#endif

    Pa_Sleep(50);

#if START_INPUT
    printf("input cpu load %2.20f\n", input.GetStreamCpuLoad());
#endif
#if START_OUTPUT
    printf("output cpu load %2.20f\n", output.GetStreamCpuLoad());
#endif

#if START_INPUT
    printf("input callback running: %d\n", input.IsCallbackRunning());
#endif
#if START_OUTPUT
    printf("output callback running: %d\n", output.IsCallbackRunning());
#endif
    printf("Exiting...\n");
    return 0;
}