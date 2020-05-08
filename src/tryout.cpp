#include "poaDecodeOutput.h"
#include "poaEncodeInput.h"
#include <memory>

#if 0
#include "../thirdparty/uvw/uvw.hpp"


void listen(uvw::Loop &loop)
{
    std::shared_ptr<uvw::TCPHandle> tcp = loop.resource<uvw::TCPHandle>();

    tcp->once<uvw::ListenEvent>([](const uvw::ListenEvent &, uvw::TCPHandle &srv) {
        std::shared_ptr<uvw::TCPHandle> client = srv.loop().resource<uvw::TCPHandle>();

        client->on<uvw::CloseEvent>([ptr = srv.shared_from_this()](const uvw::CloseEvent &, uvw::TCPHandle &) { ptr->close(); });
        client->on<uvw::EndEvent>([](const uvw::EndEvent &, uvw::TCPHandle &client) { client.close(); });

        srv.accept(*client);
        client->read();
    });

    tcp->bind("127.0.0.1", 4242);
    tcp->listen();
}

void conn(uvw::Loop &loop)
{
    auto tcp = loop.resource<uvw::TCPHandle>();

    tcp->on<uvw::ErrorEvent>([](const uvw::ErrorEvent &, uvw::TCPHandle &) { /* handle errors */ });

    tcp->once<uvw::ConnectEvent>([](const uvw::ConnectEvent &, uvw::TCPHandle &tcp) {
        auto dataWrite = std::unique_ptr<char[]>(new char[2]{'b', 'c'});
        tcp.write(std::move(dataWrite), 2);
        tcp.close();
    });

    tcp->connect(std::string{"127.0.0.1"}, 4242);
}

void tryUdp()
{
    auto loop = uvw::Loop::getDefault();
    listen(*loop);
    conn(*loop);
    //loop->run();
}
#endif

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
        /*
        cbCountErrors = 0;
        totalEncodedPacketSize = 0;
        totalFramesWritten = 0;*/

        // register callback handler
        in->registerOpusFrameAvailableCb(callbackHandler, this);
    }

    void HandleOneOpusFrameAvailable()
    {
        //printf("\nHandleOpusDataTransferCallback::HandleOneOpusFrameAvailable\n");
        cbCount++;

        bool available = in->encodedOpusFramesAvailable();
        if (!available)
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
        /*

        int read = in->readEncodedOpusFrame(transferBuffer, bufferSize);
        if (read > 0)
        {
            bool write = out->writeEncodedOpusFrame(transferBuffer, read);
            if (!write)
            {
                printf("\nHandleOneOpusFrameAvailable: FAILED writeEncodedOpusFrame at %d\n", cbCount);
            }
        }

        // this should happen once every second due to 2.5ms frame
        if (cbCount % 400 == 0)
        {
            // error
            printf("HandleOneOpusFrameAvailable cbCount: %5d, readBytes: %5d\n",
                   cbCount, read);
        }
*/
        /*

        ring_buffer_size_t framesWritten = 0;
        int encodedPacketSize = in->EncodeRecordingIntoData(transferBuffer, bufferSize);

        if (encodedPacketSize > 0)
        {
            //printf("going to DecodeDataIntoPlayback: %d\n", encodedPacketSize);
            // decode audio
            framesWritten = out->DecodeDataIntoPlayback(transferBuffer, encodedPacketSize);

            totalFramesWritten += framesWritten;
            totalEncodedPacketSize += encodedPacketSize;
        }
        else
        {
            cbCountErrors++;
        }*/
    }

private:
    poaEncodeInput *in;
    poaDecodeOutput *out;

    int bufferSize;
    void *transferBuffer;
    int cbCount;
    /*
    
    int cbCountErrors;
    int totalEncodedPacketSize;
    int totalFramesWritten;*/
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

    Pa_Sleep(10000);

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

    //    tryUdp();
    return 0;
}
