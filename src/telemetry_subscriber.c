#include "VehicleTelemetry.h"
#include "dds/dds.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SAMPLES 16U

static volatile sig_atomic_t stop_requested = 0;

static void handle_signal(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}

int main(int argc, char **argv)
{
    int domain_id = 0;
    dds_entity_t participant;
    dds_entity_t topic;
    dds_entity_t reader;
    dds_qos_t *qos;
    void *samples[MAX_SAMPLES] = {0};
    dds_sample_info_t sample_info[MAX_SAMPLES];
    size_t index;

    if (argc == 3 && strcmp(argv[1], "--domain") == 0) {
        domain_id = atoi(argv[2]);
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [--domain <id>]\n", argv[0]);
        return EXIT_FAILURE;
    }

    participant = dds_create_participant(domain_id, NULL, NULL);
    if (participant < 0) {
        fprintf(stderr, "dds_create_participant failed: %s\n", dds_strretcode(-participant));
        return EXIT_FAILURE;
    }
    topic = dds_create_topic(
        participant,
        &Vehicle_Telemetry_desc,
        "VehicleTelemetry",
        NULL,
        NULL);
    if (topic < 0) {
        fprintf(stderr, "dds_create_topic failed: %s\n", dds_strretcode(-topic));
        (void)dds_delete(participant);
        return EXIT_FAILURE;
    }

    qos = dds_create_qos();
    if (qos == NULL) {
        (void)dds_delete(participant);
        return EXIT_FAILURE;
    }
    dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_SECS(2));
    dds_qset_durability(qos, DDS_DURABILITY_TRANSIENT_LOCAL);
    dds_qset_history(qos, DDS_HISTORY_KEEP_LAST, 10);
    reader = dds_create_reader(participant, topic, qos, NULL);
    dds_delete_qos(qos);
    if (reader < 0) {
        fprintf(stderr, "dds_create_reader failed: %s\n", dds_strretcode(-reader));
        (void)dds_delete(participant);
        return EXIT_FAILURE;
    }

    for (index = 0U; index < MAX_SAMPLES; ++index) {
        samples[index] = Vehicle_Telemetry__alloc();
        if (samples[index] == NULL) {
            fprintf(stderr, "Cannot allocate DDS sample\n");
            (void)dds_delete(participant);
            return EXIT_FAILURE;
        }
    }

    (void)signal(SIGINT, handle_signal);
    (void)signal(SIGTERM, handle_signal);
    fprintf(stderr, "Waiting for VehicleTelemetry DDS samples...\n");

    while (stop_requested == 0) {
        dds_return_t count = dds_take(reader, samples, sample_info, MAX_SAMPLES, MAX_SAMPLES);
        if (count < 0) {
            fprintf(stderr, "dds_take failed: %s\n", dds_strretcode(-count));
            break;
        }
        if (count == 0) {
            dds_sleepfor(DDS_MSECS(20));
            continue;
        }
        for (index = 0U; index < (size_t)count; ++index) {
            const Vehicle_Telemetry *sample = (const Vehicle_Telemetry *)samples[index];
            if (!sample_info[index].valid_data) {
                continue;
            }
            printf(
                "source=%s pgn=%u sa=%u rpm=%s%.3f speed=%s%.3f coolant=%s%.3f\n",
                sample->source_id,
                sample->last_pgn,
                sample->source_address,
                sample->engine_speed_valid ? "" : "n/a:",
                sample->engine_speed_rpm,
                sample->vehicle_speed_valid ? "" : "n/a:",
                sample->vehicle_speed_kph,
                sample->coolant_temperature_valid ? "" : "n/a:",
                sample->coolant_temperature_c);
        }
    }

    for (index = 0U; index < MAX_SAMPLES; ++index) {
        Vehicle_Telemetry_free(samples[index], DDS_FREE_ALL);
    }
    (void)dds_delete(participant);
    return EXIT_SUCCESS;
}
