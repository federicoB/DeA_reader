#include <stdio.h>

#include "deaprotocol.h"

#define POLL_INTERVAL 60

int main() {

    struct usb_dev_handle* devh;

    devh = deaInit();
    while (1) {
        deaReadData(devh);
        sleep(POLL_INTERVAL);
    }
    deaExit(devh);

    return 0;
}
