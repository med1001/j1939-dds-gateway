#include "j1939_dds/decoder.h"
#include "j1939_dds/source.h"

#include <math.h>
#include <stdio.h>

#ifndef TEST_FIXTURE_PATH
#error "TEST_FIXTURE_PATH must point to the replay fixture"
#endif

typedef struct {
    vehicle_telemetry_t telemetry;
    unsigned int received;
    unsigned int decoded;
} pipeline_test_context_t;

static int process_message(const j1939_message_t *message, void *user_data)
{
    pipeline_test_context_t *context = (pipeline_test_context_t *)user_data;
    j1939_decode_result_t result;

    ++context->received;
    result = j1939_decode_message(message, &context->telemetry);
    if (result == J1939_DECODE_UPDATED) {
        ++context->decoded;
    }
    return 0;
}
int main(void)
{
    pipeline_test_context_t context;

    context.received = 0U;
    context.decoded = 0U;
    vehicle_telemetry_init(&context.telemetry, "integration-test");

    if (replay_source_run(TEST_FIXTURE_PATH, process_message, &context) != 0) {
        fprintf(stderr, "Replay source failed\n");
        return 1;
    }
    if (context.received != 4U || context.decoded != 3U) {
        fprintf(
            stderr,
            "Unexpected pipeline counts: received=%u decoded=%u\n",
            context.received,
            context.decoded);
        return 1;
    }
    if (!context.telemetry.engine_speed_valid ||
        fabs(context.telemetry.engine_speed_rpm - 1000.0) > 0.0001 ||
        !context.telemetry.vehicle_speed_valid ||
        fabs(context.telemetry.vehicle_speed_kph - 80.0) > 0.0001 ||
        !context.telemetry.coolant_temperature_valid ||
        fabs(context.telemetry.coolant_temperature_c - 80.0) > 0.0001) {
        fprintf(stderr, "Decoded telemetry state is incorrect\n");
        return 1;
    }

    printf("Replay-to-decoder integration test passed\n");
    return 0;
}
