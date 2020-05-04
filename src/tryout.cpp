#include "poaDecodeOutput.h"
#include "poaEncodeInput.h"

int tryout()
{
    poaEncodeInput input("input");
    poaDecodeOutput output("output");

    input.log("testing %d\n", 1);
    output.log("testing %s\n", "foo");

    input.OpenInputDeviceStream();
    output.OpenOutputDeviceStream();

    return 0;
}