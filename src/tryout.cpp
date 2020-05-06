#include <string.h>
#include <cstring>
#include "poaDecodeOutput.h"
#include "poaEncodeInput.h"

#include "../thirdparty/uvw/uvw.hpp"

#include <memory>

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
    HandleUdpDuplexCallback(uvw::Loop &loop, poaEncodeInput *input, poaDecodeOutput *output)
    {
        in = input;
        out = output;
        //bufferSize = 10240;                  //in->GetUncompressedBufferSizeBytes();
        //transferBuffer = malloc(bufferSize); //ALLIGNEDMALLOC(bufferSize);
        cbCount = 0;

        // register callback handler
        in->registerOpusFrameAvailableCb(callbackHandler, this);

        // TODO: running this on the node loop will not run that async receive
        // until default loop can process stuff

        server = loop.resource<uvw::UDPHandle>();
        client = loop.resource<uvw::UDPHandle>();

        server->on<uvw::ErrorEvent>([](const uvw::ErrorEvent &e, uvw::UDPHandle &) {
            printf("server Error: %d %s\n", e.code(), e.what());
        });
        client->on<uvw::ErrorEvent>([](const uvw::ErrorEvent &e, uvw::UDPHandle &) {
            printf("client Error: %d %s\n", e.code(), e.what());
        });

        server->on<uvw::UDPDataEvent>([this](const uvw::UDPDataEvent &e, uvw::UDPHandle &h) {
            this->uvwHandleUDPDataEvent(e, h);
        });

        server->bind(address, portRecv);
        server->recv();
    }

    void uvwHandleUDPDataEvent(const uvw::UDPDataEvent &e, uvw::UDPHandle &h)
    {
        if (e.length != sizeof(poaCallbackTransferData))
        {
            printf("uvwHandleUDPDataEvent UNEXPECTED size %ld\n", e.length);
            return;
        }

        poaCallbackTransferData *payload = (poaCallbackTransferData *)e.data.get();
        bool written = out->writeEncodedOpusFrame(payload);
        if (!written)
        {
            printf("HandleOneOpusFrameAvailable could not writeEncodedOpusFrame??\n");
        }
    }

    void uvwHandleErrorEvent(const uvw::ErrorEvent &e, uvw::UDPHandle &)
    {
        printf("UVW ERROR: %d %s\n", e.code(), e.what());
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
        send(&data);
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

    void send(const poaCallbackTransferData *payload)
    {
        //printf("send (%d)\n", sendCount);
        std::unique_ptr<char[]> data{new char[sizeof(poaCallbackTransferData)]};
        std::memcpy(data.get(), payload, sizeof(poaCallbackTransferData));
        client->send(address, portRecv, std::move(data), sizeof(poaCallbackTransferData));
        sendCount++;
    }

    poaEncodeInput *in;
    poaDecodeOutput *out;
    int cbCount;

    int sendCount;

    std::shared_ptr<uvw::UDPHandle> server;
    std::shared_ptr<uvw::UDPHandle> client;

    const std::string address = std::string{"127.0.0.1"};
    const unsigned int portRecv = 2223;
};

// temp workaround for getting to nodejs eventloop outside tryout
poaEncodeInput *input = NULL;
poaDecodeOutput *output = NULL;

HandleUdpDuplexCallback *recordingHandler = NULL;

int tryout()
{
#if START_INPUT
    input = new poaEncodeInput("input");
    input->log("testing %d\n", 1);
#endif
#if START_OUTPUT
    output = new poaDecodeOutput("output");
    output->log("testing %s\n", "foo");
#endif

#define USE_UDP 1
#if USE_UDP
    // for now cannot use default loop since that is nodejs one and we would need to exit this tryout function for it to do work again
    auto loop = uvw::Loop::getDefault();
    recordingHandler = new HandleUdpDuplexCallback(*loop, input, output);
#else
    // only works if START_INPUT and START_OUTPUT are defined
    HandleOpusDataTransferCallback recordingHandler(&input, &output);
#endif

    PaError err;

#if START_INPUT
    err = input->OpenInputDeviceStream();
    LOGERR(err, "input.OpenInputDeviceStream");
#endif

#if START_OUTPUT
    err = output->OpenOutputDeviceStream();
    LOGERR(err, "output.OpenOutputDeviceStream");
#endif

#if START_INPUT
    printf("input callback running: %d\n", input->IsCallbackRunning());
#endif
#if START_OUTPUT
    printf("output callback running: %d\n", output->IsCallbackRunning());
#endif

    printf("StartStream\n");
#if START_INPUT
    err = input->StartStream();
    LOGERR(err, "input.StartStream");
#endif
#if START_OUTPUT
    err = output->StartStream();
    LOGERR(err, "output.StartStream");
#endif

#if USE_UDP
    //loop->run();
#endif
    return 0;
}

#if 0
int next() {}
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

    tryUdp();
    return 0;
}
#endif