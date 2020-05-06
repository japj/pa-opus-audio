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

class HandleOpusDataTransferCallback
{
    // callback handler that is called from paEncodeInstream to notify that an opus frame is available
    static void callbackHandler(void *userData)
    {
        HandleOpusDataTransferCallback *p = (HandleOpusDataTransferCallback *)userData;
        p->HandleOneOpusFrameAvailable();
    }

public:
    HandleOpusDataTransferCallback(poaEncodeInput *input, poaDecodeOutput *output)
    {
        in = input;
        out = output;
        bufferSize = 10240;                  //in->GetUncompressedBufferSizeBytes();
        transferBuffer = malloc(bufferSize); //ALLIGNEDMALLOC(bufferSize);
        cbCount = 0;

        // register callback handler
        in->registerOpusFrameAvailableCb(callbackHandler, this);
    }

    void HandleOneOpusFrameAvailable()
    {
        //printf("\nHandleOpusDataTransferCallback::HandleOneOpusFrameAvailable\n");
        cbCount++;

        int available = in->encodedOpusFramesAvailable();
        if (available < 1)
        {
            printf("HandleOneOpusFrameAvailable got trigger, but no frame available??? \n");
            return;
        }

        poaCallbackTransferData data;
        bool read = in->readEncodedOpusFrame(&data);
        if (!read)
        {
            printf("HandleOneOpusFrameAvailable could not readEncodedOpusFrame??\n");
            return;
        }
        //printf("HandleOneOpusFrameAvailable sequenceNumber(%d) dataLength(%ld) \n", data.sequenceNumber, data.dataLength);

        bool written = out->writeEncodedOpusFrame(&data);
        if (!written)
        {
            printf("HandleOneOpusFrameAvailable could not writeEncodedOpusFrame??\n");
        }
    }

private:
    poaEncodeInput *in;
    poaDecodeOutput *out;

    int bufferSize;
    void *transferBuffer;
    int cbCount;
};

class HandleUdpDuplexCallback
{
    // callback handler that is called from paEncodeInstream to notify that an opus frame is available
    static void callbackHandler(void *userData)
    {
        HandleUdpDuplexCallback *p = (HandleUdpDuplexCallback *)userData;
        p->HandleOneOpusFrameAvailableFromInput();
    }

public:
    HandleUdpDuplexCallback(poaEncodeInput *input, poaDecodeOutput *output)
    {
        in = input;
        out = output;
        //bufferSize = 10240;                  //in->GetUncompressedBufferSizeBytes();
        //transferBuffer = malloc(bufferSize); //ALLIGNEDMALLOC(bufferSize);
        cbCount = 0;

        // register callback handler
        in->registerOpusFrameAvailableCb(callbackHandler, this);
    }

    void HandleOneOpusFrameAvailableFromInput()
    {
        //printf("\nHandleOpusDataTransferCallback::HandleOneOpusFrameAvailable\n");
        cbCount++;

        int available = in->encodedOpusFramesAvailable();
        if (available < 1)
        {
            printf("HandleOneOpusFrameAvailable got trigger, but no frame available??? \n");
            return;
        }

        poaCallbackTransferData data;
        bool read = in->readEncodedOpusFrame(&data);
        if (!read)
        {
            printf("HandleOneOpusFrameAvailable could not readEncodedOpusFrame??\n");
            return;
        }

        // TODO write to UDP socket
    }

    // TODO: receive from UDP socket
    void HandleOneOpusFrameAvailableFromUDP(poaCallbackTransferData *data)
    {
        bool written = out->writeEncodedOpusFrame(data);
        if (!written)
        {
            printf("HandleOneOpusFrameAvailable could not writeEncodedOpusFrame??\n");
        }
    }

    poaEncodeInput *in;
    poaDecodeOutput *out;
    int cbCount;
};

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

    // only works if START_INPUT and START_OUTPUT are defined
    HandleOpusDataTransferCallback recordingHandler(&input, &output);

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

    Pa_Sleep(5000);

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