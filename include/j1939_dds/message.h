#ifndef J1939_DDS_MESSAGE_H
#define J1939_DDS_MESSAGE_H

#include <stddef.h>
#include <stdint.h>

#define J1939_DDS_MAX_PAYLOAD 1785U

typedef struct {
    uint64_t timestamp_ms;
    uint32_t pgn;
    uint8_t source_address;
    size_t payload_length;
    uint8_t payload[J1939_DDS_MAX_PAYLOAD];
} j1939_message_t;

typedef int (*j1939_message_callback_t)(const j1939_message_t *message, void *user_data);

#endif
