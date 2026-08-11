#ifndef J1939_DDS_PUBLISHER_H
#define J1939_DDS_PUBLISHER_H

#include "j1939_dds/telemetry.h"

typedef struct telemetry_publisher telemetry_publisher_t;

struct telemetry_publisher {
    void *context;
    int (*publish)(telemetry_publisher_t *publisher, const vehicle_telemetry_t *telemetry);
    void (*destroy)(telemetry_publisher_t *publisher);
};

int stdout_publisher_create(telemetry_publisher_t *publisher);

#if defined(J1939_DDS_HAVE_CYCLONEDDS)
int cyclone_publisher_create(telemetry_publisher_t *publisher, int domain_id);
#endif
#endif
