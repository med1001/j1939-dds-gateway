#include "j1939_dds/decoder.h"

#include <stdio.h>
#include <string.h>

static uint16_t read_u16_le(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8U));
}

void vehicle_telemetry_init(vehicle_telemetry_t *telemetry, const char *source_id)
{
    if (telemetry == NULL) {
        return;
    }

    memset(telemetry, 0, sizeof(*telemetry));
    if (source_id != NULL) {
        (void)snprintf(
            telemetry->source_id,
            sizeof(telemetry->source_id),
            "%s",
            source_id);
    }
}

j1939_decode_result_t j1939_decode_message(
    const j1939_message_t *message,
    vehicle_telemetry_t *telemetry)
{
    uint16_t raw_value;

    if (message == NULL || telemetry == NULL) {
        return J1939_DECODE_INVALID_ARGUMENT;
    }

    telemetry->timestamp_ms = message->timestamp_ms;
    telemetry->last_pgn = message->pgn;
    telemetry->source_address = message->source_address;

    switch (message->pgn) {
    case J1939_PGN_EEC1:
        if (message->payload_length < 5U) {
            return J1939_DECODE_INVALID_LENGTH;
        }
        raw_value = read_u16_le(&message->payload[3]);
        telemetry->engine_speed_valid = raw_value != UINT16_MAX;
        if (telemetry->engine_speed_valid) {
            telemetry->engine_speed_rpm = (double)raw_value * 0.125;
        }
        return J1939_DECODE_UPDATED;

    case J1939_PGN_CCVS1:
        if (message->payload_length < 3U) {
            return J1939_DECODE_INVALID_LENGTH;
        }
        raw_value = read_u16_le(&message->payload[1]);
        telemetry->vehicle_speed_valid = raw_value != UINT16_MAX;
        if (telemetry->vehicle_speed_valid) {
            telemetry->vehicle_speed_kph = (double)raw_value / 256.0;
        }
        return J1939_DECODE_UPDATED;

    case J1939_PGN_ET1:
        if (message->payload_length < 1U) {
            return J1939_DECODE_INVALID_LENGTH;
        }
        telemetry->coolant_temperature_valid = message->payload[0] != UINT8_MAX;
        if (telemetry->coolant_temperature_valid) {
            telemetry->coolant_temperature_c = (double)message->payload[0] - 40.0;
        }
        return J1939_DECODE_UPDATED;

    default:
        return J1939_DECODE_UNSUPPORTED;
    }
}
