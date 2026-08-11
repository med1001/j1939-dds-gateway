#include "j1939_dds/source.h"

#include <errno.h>
#include <linux/can.h>
#include <linux/can/j1939.h>
#include <net/if.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static uint64_t current_time_ms(void)
{
    struct timespec time_value;

    if (timespec_get(&time_value, TIME_UTC) != TIME_UTC) {
        return 0U;
    }
    return ((uint64_t)time_value.tv_sec * 1000U) + ((uint64_t)time_value.tv_nsec / 1000000U);
}

int linux_j1939_source_run(
    const char *interface_name,
    j1939_message_callback_t callback,
    void *user_data)
{
    int socket_fd;
    int broadcast_enabled = 1;
    unsigned int interface_index;
    struct sockaddr_can bind_address;

    if (interface_name == NULL || callback == NULL) {
        return -1;
    }

    interface_index = if_nametoindex(interface_name);
    if (interface_index == 0U) {
        fprintf(stderr, "Unknown CAN interface '%s'\n", interface_name);
        return -1;
    }

    socket_fd = socket(PF_CAN, SOCK_DGRAM, CAN_J1939);
    if (socket_fd < 0) {
        fprintf(stderr, "Cannot create CAN J1939 socket: %s\n", strerror(errno));
        return -1;
    }

    if (setsockopt(
            socket_fd,
            SOL_SOCKET,
            SO_BROADCAST,
            &broadcast_enabled,
            sizeof(broadcast_enabled)) != 0) {
        fprintf(stderr, "Cannot enable J1939 broadcast reception: %s\n", strerror(errno));
        (void)close(socket_fd);
        return -1;
    }

    memset(&bind_address, 0, sizeof(bind_address));
    bind_address.can_family = AF_CAN;
    bind_address.can_ifindex = (int)interface_index;
    bind_address.can_addr.j1939.name = J1939_NO_NAME;
    bind_address.can_addr.j1939.addr = J1939_NO_ADDR;
    bind_address.can_addr.j1939.pgn = J1939_NO_PGN;

    if (bind(
            socket_fd,
            (const struct sockaddr *)&bind_address,
            sizeof(bind_address)) != 0) {
        fprintf(stderr, "Cannot bind J1939 socket to '%s': %s\n", interface_name, strerror(errno));
        (void)close(socket_fd);
        return -1;
    }

    fprintf(stderr, "Listening for CAN J1939 traffic on %s...\n", interface_name);
    for (;;) {
        j1939_message_t message;
        struct sockaddr_can source_address;
        socklen_t address_length = sizeof(source_address);
        ssize_t received;

        memset(&message, 0, sizeof(message));
        memset(&source_address, 0, sizeof(source_address));
        received = recvfrom(
            socket_fd,
            message.payload,
            sizeof(message.payload),
            0,
            (struct sockaddr *)&source_address,
            &address_length);

        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "J1939 receive error: %s\n", strerror(errno));
            (void)close(socket_fd);
            return -1;
        }

        message.timestamp_ms = current_time_ms();
        message.pgn = source_address.can_addr.j1939.pgn;
        message.source_address = source_address.can_addr.j1939.addr;
        message.payload_length = (size_t)received;
        if (callback(&message, user_data) != 0) {
            break;
        }
    }

    return close(socket_fd) == 0 ? 0 : -1;
}
