#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "app_program/subsystem1/s1_bridge.h"

int main(void)
{
    const uint8_t direction[] = {0x27, 71, 1};
    uint8_t received[S1_FRAME_MAX];
    s1_bridge_link(true, true);
    s1_bridge_put(direction, sizeof direction);
    s1_bridge_link(false, false);
    unsigned length = s1_bridge_receive(received, sizeof received);
    assert(!s1_bridge_ready());
    assert(length == sizeof direction && memcmp(direction, received, length) == 0);
    printf("Offline RX still delivers queued direction: length=%u sequence=%u action=%u\n",
        length, received[1], received[2]);
}
