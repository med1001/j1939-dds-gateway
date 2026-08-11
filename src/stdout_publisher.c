#include "j1939_dds/publisher.h"

#include <stdio.h>
#include <string.h>

static int publish_to_stdout(
    telemetry_publisher_t *publisher,
    const vehicle_telemetry_t *telemetry)
{
    (void)publisher;

    if (telemetry == NULL) {
        return -1;
    }

    printf(
        "{\"source_id\":\"%s\",\"timestamp_ms\":%llu,\"pgn\":%u,"
        "\"source_address\":%u,\"engine_speed_rpm\":",
        telemetry->source_id,
        (unsigned long long)telemetry->timestamp_ms,
        telemetry->last_pgn,
        telemetry->source_address);

    if (telemetry->engine_speed_valid) {
        printf("%.3f", telemetry->engine_speed_rpm);
    } else {
        printf("null");
    }

    printf(",\"vehicle_speed_kph\":");
    if (telemetry->vehicle_speed_valid) {
        printf("%.3f", telemetry->vehicle_speed_kph);
    } else {
        printf("null");
    }

    printf(",\"coolant_temperature_c\":");
    if (telemetry->coolant_temperature_valid) {
        printf("%.3f", telemetry->coolant_temperature_c);
    } else {
        printf("null");
    }
    printf("}\n");
    return fflush(stdout) == 0 ? 0 : -1;
}
static void destroy_stdout_publisher(telemetry_publisher_t *publisher)
{
    if (publisher != NULL) {
        memset(publisher, 0, sizeof(*publisher));
    }
}

int stdout_publisher_create(telemetry_publisher_t *publisher)
{
    if (publisher == NULL) {
        return -1;
    }
    publisher->context = NULL;
    publisher->publish = publish_to_stdout;
    publisher->destroy = destroy_stdout_publisher;
    return 0;
}
