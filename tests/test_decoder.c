#include "j1939_dds/decoder.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures = 0;

#define EXPECT_TRUE(condition)                                                          \
    do {                                                                                \
        if (!(condition)) {                                                             \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);      \
            ++failures;                                                                 \
        }                                                                               \
    } while (0)

#define EXPECT_NEAR(actual, expected, tolerance)                                        \
    do {                                                                                \
        double actual_value = (actual);                                                 \
        double expected_value = (expected);                                             \
        if (fabs(actual_value - expected_value) > (tolerance)) {                        \
            fprintf(                                                                   \
                stderr,                                                                 \
                "FAIL %s:%d: %.6f != %.6f\n",                                         \
                __FILE__,                                                               \
                __LINE__,                                                               \
                actual_value,                                                           \
                expected_value);                                                        \
            ++failures;                                                                 \
        }                                                                               \
    } while (0)

static j1939_message_t make_message(uint32_t pgn, const uint8_t *payload, size_t length)
{
    j1939_message_t message;

    memset(&message, 0, sizeof(message));
    message.timestamp_ms = 123456U;
    message.pgn = pgn;
    message.source_address = 0x80U;
    message.payload_length = length;
    if (payload != NULL && length <= sizeof(message.payload)) {
        memcpy(message.payload, payload, length);
    }
    return message;
}
static void test_initialization(void)
{
    vehicle_telemetry_t telemetry;

    vehicle_telemetry_init(&telemetry, "airport-tractor-01");
    EXPECT_TRUE(strcmp(telemetry.source_id, "airport-tractor-01") == 0);
    EXPECT_TRUE(!telemetry.engine_speed_valid);
    EXPECT_TRUE(!telemetry.vehicle_speed_valid);
    EXPECT_TRUE(!telemetry.coolant_temperature_valid);
}

static void test_engine_speed(void)
{
    const uint8_t payload[8] = {0xFFU, 0xFFU, 0xFFU, 0x40U, 0x1FU, 0xFFU, 0xFFU, 0xFFU};
    j1939_message_t message = make_message(J1939_PGN_EEC1, payload, sizeof(payload));
    vehicle_telemetry_t telemetry;

    vehicle_telemetry_init(&telemetry, "test");
    EXPECT_TRUE(j1939_decode_message(&message, &telemetry) == J1939_DECODE_UPDATED);
    EXPECT_TRUE(telemetry.engine_speed_valid);
    EXPECT_NEAR(telemetry.engine_speed_rpm, 1000.0, 0.0001);
    EXPECT_TRUE(telemetry.last_pgn == J1939_PGN_EEC1);
    EXPECT_TRUE(telemetry.source_address == 0x80U);
}

static void test_vehicle_speed(void)
{
    const uint8_t payload[8] = {0xFFU, 0x00U, 0x50U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    j1939_message_t message = make_message(J1939_PGN_CCVS1, payload, sizeof(payload));
    vehicle_telemetry_t telemetry;

    vehicle_telemetry_init(&telemetry, "test");
    EXPECT_TRUE(j1939_decode_message(&message, &telemetry) == J1939_DECODE_UPDATED);
    EXPECT_TRUE(telemetry.vehicle_speed_valid);
    EXPECT_NEAR(telemetry.vehicle_speed_kph, 80.0, 0.0001);
}

static void test_coolant_temperature(void)
{
    const uint8_t payload[8] = {0x78U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    j1939_message_t message = make_message(J1939_PGN_ET1, payload, sizeof(payload));
    vehicle_telemetry_t telemetry;

    vehicle_telemetry_init(&telemetry, "test");
    EXPECT_TRUE(j1939_decode_message(&message, &telemetry) == J1939_DECODE_UPDATED);
    EXPECT_TRUE(telemetry.coolant_temperature_valid);
    EXPECT_NEAR(telemetry.coolant_temperature_c, 80.0, 0.0001);
}

static void test_not_available_values(void)
{
    const uint8_t payload[8] = {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    j1939_message_t message = make_message(J1939_PGN_EEC1, payload, sizeof(payload));
    vehicle_telemetry_t telemetry;

    vehicle_telemetry_init(&telemetry, "test");
    EXPECT_TRUE(j1939_decode_message(&message, &telemetry) == J1939_DECODE_UPDATED);
    EXPECT_TRUE(!telemetry.engine_speed_valid);

    message.pgn = J1939_PGN_CCVS1;
    EXPECT_TRUE(j1939_decode_message(&message, &telemetry) == J1939_DECODE_UPDATED);
    EXPECT_TRUE(!telemetry.vehicle_speed_valid);

    message.pgn = J1939_PGN_ET1;
    EXPECT_TRUE(j1939_decode_message(&message, &telemetry) == J1939_DECODE_UPDATED);
    EXPECT_TRUE(!telemetry.coolant_temperature_valid);
}

static void test_errors_and_unsupported_pgn(void)
{
    const uint8_t payload[2] = {0U, 0U};
    j1939_message_t message = make_message(J1939_PGN_EEC1, payload, sizeof(payload));
    vehicle_telemetry_t telemetry;

    vehicle_telemetry_init(&telemetry, "test");
    EXPECT_TRUE(j1939_decode_message(&message, &telemetry) == J1939_DECODE_INVALID_LENGTH);
    message.pgn = 12345U;
    EXPECT_TRUE(j1939_decode_message(&message, &telemetry) == J1939_DECODE_UNSUPPORTED);
    EXPECT_TRUE(j1939_decode_message(NULL, &telemetry) == J1939_DECODE_INVALID_ARGUMENT);
    EXPECT_TRUE(j1939_decode_message(&message, NULL) == J1939_DECODE_INVALID_ARGUMENT);
}

int main(void)
{
    test_initialization();
    test_engine_speed();
    test_vehicle_speed();
    test_coolant_temperature();
    test_not_available_values();
    test_errors_and_unsupported_pgn();

    if (failures != 0) {
        fprintf(stderr, "%d decoder test(s) failed\n", failures);
        return 1;
    }
    printf("All decoder tests passed\n");
    return 0;
}
