#include <stdio.h>

#include "deaprotocol.h"



int main() {

    struct usb_dev_handle* devh;

    devh = deaInit();
    while (1)
        deaReadData(devh);
    deaExit(devh);

    return 0;
}
