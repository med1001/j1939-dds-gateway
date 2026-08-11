#ifndef J1939_DDS_SOURCE_H
#define J1939_DDS_SOURCE_H

#include "j1939_dds/message.h"

int replay_source_run(
    const char *path,
    j1939_message_callback_t callback,
    void *user_data);

#if defined(J1939_DDS_HAVE_LINUX_J1939)
int linux_j1939_source_run(
    const char *interface_name,
    j1939_message_callback_t callback,
    void *user_data);
#endif
#endif
