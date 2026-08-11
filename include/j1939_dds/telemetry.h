#ifndef J1939_DDS_TELEMETRY_H
#define J1939_DDS_TELEMETRY_H

#include <stdbool.h>
#include <stdint.h>

#define J1939_DDS_SOURCE_ID_LENGTH 32U

typedef struct {
    char source_id[J1939_DDS_SOURCE_ID_LENGTH];
    uint64_t timestamp_ms;
    uint32_t last_pgn;
    uint8_t source_address;
    bool engine_speed_valid;
    double engine_speed_rpm;
    bool vehicle_speed_valid;
    double vehicle_speed_kph;
    bool coolant_temperature_valid;
    double coolant_temperature_c;
} vehicle_telemetry_t;

void vehicle_telemetry_init(vehicle_telemetry_t *telemetry, const char *source_id);

#endif
