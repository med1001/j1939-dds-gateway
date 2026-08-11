#include "j1939_dds/publisher.h"

#include "VehicleTelemetry.h"
#include "dds/dds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    dds_entity_t participant;
    dds_entity_t writer;
} cyclone_context_t;

static int publish_to_cyclone(
    telemetry_publisher_t *publisher,
    const vehicle_telemetry_t *telemetry)
{
    cyclone_context_t *context;
    Vehicle_Telemetry sample;
    dds_return_t result;

    if (publisher == NULL || publisher->context == NULL || telemetry == NULL) {
        return -1;
    }

    context = (cyclone_context_t *)publisher->context;
    memset(&sample, 0, sizeof(sample));
    (void)snprintf(sample.source_id, sizeof(sample.source_id), "%s", telemetry->source_id);
    sample.timestamp_ms = telemetry->timestamp_ms;
    sample.last_pgn = telemetry->last_pgn;
    sample.source_address = telemetry->source_address;
    sample.engine_speed_valid = telemetry->engine_speed_valid;
    sample.engine_speed_rpm = telemetry->engine_speed_rpm;
    sample.vehicle_speed_valid = telemetry->vehicle_speed_valid;
    sample.vehicle_speed_kph = telemetry->vehicle_speed_kph;
    sample.coolant_temperature_valid = telemetry->coolant_temperature_valid;
    sample.coolant_temperature_c = telemetry->coolant_temperature_c;

    result = dds_write(context->writer, &sample);
    if (result != DDS_RETCODE_OK) {
        fprintf(stderr, "dds_write failed: %s\n", dds_strretcode(-result));
        return -1;
    }
    return 0;
}

static void destroy_cyclone_publisher(telemetry_publisher_t *publisher)
{
    cyclone_context_t *context;

    if (publisher == NULL) {
        return;
    }
    context = (cyclone_context_t *)publisher->context;
    if (context != NULL) {
        if (context->participant >= 0) {
            (void)dds_delete(context->participant);
        }
        free(context);
    }
    memset(publisher, 0, sizeof(*publisher));
}

int cyclone_publisher_create(telemetry_publisher_t *publisher, int domain_id)
{
    cyclone_context_t *context;
    dds_entity_t topic;
    dds_qos_t *qos;

    if (publisher == NULL) {
        return -1;
    }

    context = (cyclone_context_t *)calloc(1U, sizeof(*context));
    if (context == NULL) {
        return -1;
    }
    context->participant = DDS_RETCODE_ERROR;
    context->writer = DDS_RETCODE_ERROR;

    context->participant = dds_create_participant(domain_id, NULL, NULL);
    if (context->participant < 0) {
        fprintf(stderr, "dds_create_participant failed: %s\n", dds_strretcode(-context->participant));
        free(context);
        return -1;
    }

    topic = dds_create_topic(
        context->participant,
        &Vehicle_Telemetry_desc,
        "VehicleTelemetry",
        NULL,
        NULL);
    if (topic < 0) {
        fprintf(stderr, "dds_create_topic failed: %s\n", dds_strretcode(-topic));
        (void)dds_delete(context->participant);
        free(context);
        return -1;
    }

    qos = dds_create_qos();
    if (qos == NULL) {
        (void)dds_delete(context->participant);
        free(context);
        return -1;
    }
    dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_SECS(2));
    dds_qset_durability(qos, DDS_DURABILITY_TRANSIENT_LOCAL);
    dds_qset_history(qos, DDS_HISTORY_KEEP_LAST, 10);
    context->writer = dds_create_writer(context->participant, topic, qos, NULL);
    dds_delete_qos(qos);
    if (context->writer < 0) {
        fprintf(stderr, "dds_create_writer failed: %s\n", dds_strretcode(-context->writer));
        (void)dds_delete(context->participant);
        free(context);
        return -1;
    }

    publisher->context = context;
    publisher->publish = publish_to_cyclone;
    publisher->destroy = destroy_cyclone_publisher;
    return 0;
}
