#ifndef J1939_DDS_DECODER_H
#define J1939_DDS_DECODER_H

#include "j1939_dds/message.h"
#include "j1939_dds/telemetry.h"

#define J1939_PGN_EEC1 61444U
#define J1939_PGN_CCVS1 65265U
#define J1939_PGN_ET1 65262U

typedef enum {
    J1939_DECODE_UPDATED = 0,
    J1939_DECODE_UNSUPPORTED = 1,
    J1939_DECODE_INVALID_ARGUMENT = -1,
    J1939_DECODE_INVALID_LENGTH = -2
} j1939_decode_result_t;

j1939_decode_result_t j1939_decode_message(
    const j1939_message_t *message,
    vehicle_telemetry_t *telemetry);

#endif
