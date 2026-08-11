#include "j1939_dds/decoder.h"

#include <errno.h>
#include <linux/can.h>
#include <linux/can/j1939.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int send_pgn(int socket_fd, uint32_t pgn, const uint8_t *payload, size_t length)
{
    struct sockaddr_can destination;
    ssize_t written;

    memset(&destination, 0, sizeof(destination));
    destination.can_family = AF_CAN;
    destination.can_addr.j1939.name = J1939_NO_NAME;
    destination.can_addr.j1939.addr = J1939_NO_ADDR;
    destination.can_addr.j1939.pgn = pgn;

    written = sendto(
        socket_fd,
        payload,
        length,
        0,
        (const struct sockaddr *)&destination,
        sizeof(destination));
    if (written != (ssize_t)length) {
        fprintf(stderr, "Cannot send PGN %u: %s\n", pgn, strerror(errno));
        return -1;
    }
    printf("Sent PGN %u (%zu bytes)\n", pgn, length);
    return 0;
}

int main(int argc, char **argv)
{
    const uint8_t eec1_payload[8] = {0xFFU, 0xFFU, 0xFFU, 0x40U, 0x1FU, 0xFFU, 0xFFU, 0xFFU};
    const uint8_t ccvs1_payload[8] = {0xFFU, 0x00U, 0x50U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    const uint8_t et1_payload[8] = {0x78U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    unsigned int interface_index;
    int socket_fd;
    int broadcast_enabled = 1;
    struct sockaddr_can source;
    int status = EXIT_FAILURE;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <can-interface>\n", argv[0]);
        return EXIT_FAILURE;
    }

    interface_index = if_nametoindex(argv[1]);
    if (interface_index == 0U) {
        fprintf(stderr, "Unknown CAN interface '%s'\n", argv[1]);
        return EXIT_FAILURE;
    }

    socket_fd = socket(PF_CAN, SOCK_DGRAM, CAN_J1939);
    if (socket_fd < 0) {
        fprintf(stderr, "Cannot create CAN J1939 socket: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    if (setsockopt(
            socket_fd,
            SOL_SOCKET,
            SO_BROADCAST,
            &broadcast_enabled,
            sizeof(broadcast_enabled)) != 0) {
        fprintf(stderr, "Cannot enable broadcast: %s\n", strerror(errno));
        (void)close(socket_fd);
        return EXIT_FAILURE;
    }

    memset(&source, 0, sizeof(source));
    source.can_family = AF_CAN;
    source.can_ifindex = (int)interface_index;
    source.can_addr.j1939.name = J1939_NO_NAME;
    source.can_addr.j1939.addr = 0x80U;
    source.can_addr.j1939.pgn = J1939_NO_PGN;
    if (bind(socket_fd, (const struct sockaddr *)&source, sizeof(source)) != 0) {
        fprintf(stderr, "Cannot bind sender to '%s': %s\n", argv[1], strerror(errno));
        (void)close(socket_fd);
        return EXIT_FAILURE;
    }

    if (send_pgn(socket_fd, J1939_PGN_EEC1, eec1_payload, sizeof(eec1_payload)) == 0 &&
        send_pgn(socket_fd, J1939_PGN_CCVS1, ccvs1_payload, sizeof(ccvs1_payload)) == 0 &&
        send_pgn(socket_fd, J1939_PGN_ET1, et1_payload, sizeof(et1_payload)) == 0) {
        status = EXIT_SUCCESS;
    }

    (void)close(socket_fd);
    return status;
}
