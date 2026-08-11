#include "j1939_dds/decoder.h"
#include "j1939_dds/publisher.h"
#include "j1939_dds/source.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    vehicle_telemetry_t telemetry;
    telemetry_publisher_t publisher;
    unsigned long decoded_messages;
    unsigned long unsupported_messages;
    unsigned long invalid_messages;
} gateway_context_t;

static volatile sig_atomic_t stop_requested = 0;

static void handle_signal(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}

static int install_signal_handlers(void)
{
#if defined(J1939_DDS_HAVE_LINUX_J1939)
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_signal;
    if (sigemptyset(&action.sa_mask) != 0 ||
        sigaction(SIGINT, &action, NULL) != 0 ||
        sigaction(SIGTERM, &action, NULL) != 0) {
        return -1;
    }
    return 0;
#else
    return signal(SIGINT, handle_signal) == SIG_ERR ||
                   signal(SIGTERM, handle_signal) == SIG_ERR
               ? -1
               : 0;
#endif
}

static void print_usage(const char *program_name)
{
    printf(
        "Usage:\n"
        "  %s --replay <csv> [--publisher stdout|dds] [--domain <id>] [--source-id <id>]\n"
        "  %s --interface <can-iface> [--publisher stdout|dds] [--domain <id>] [--source-id <id>]\n",
        program_name,
        program_name);
}

static int process_message(const j1939_message_t *message, void *user_data)
{
    gateway_context_t *gateway = (gateway_context_t *)user_data;
    j1939_decode_result_t result;

    if (stop_requested != 0) {
        return 1;
    }

    result = j1939_decode_message(message, &gateway->telemetry);
    if (result == J1939_DECODE_UNSUPPORTED) {
        ++gateway->unsupported_messages;
        return 0;
    }
    if (result != J1939_DECODE_UPDATED) {
        ++gateway->invalid_messages;
        fprintf(stderr, "Invalid J1939 payload for PGN %u\n", message->pgn);
        return 0;
    }

    ++gateway->decoded_messages;
    return gateway->publisher.publish(&gateway->publisher, &gateway->telemetry);
}

int main(int argc, char **argv)
{
    const char *replay_path = NULL;
    const char *interface_name = NULL;
    const char *publisher_name = "stdout";
    const char *source_id = "gateway-01";
    int domain_id = 0;
    int index;
    int status;
    gateway_context_t gateway;

    memset(&gateway, 0, sizeof(gateway));

    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--help") == 0 || strcmp(argv[index], "-h") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        }
        if (index + 1 >= argc) {
            fprintf(stderr, "Missing value after '%s'\n", argv[index]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
        if (strcmp(argv[index], "--replay") == 0) {
            replay_path = argv[++index];
        } else if (strcmp(argv[index], "--interface") == 0) {
            interface_name = argv[++index];
        } else if (strcmp(argv[index], "--publisher") == 0) {
            publisher_name = argv[++index];
        } else if (strcmp(argv[index], "--source-id") == 0) {
            source_id = argv[++index];
        } else if (strcmp(argv[index], "--domain") == 0) {
            char *end = NULL;
            long parsed = strtol(argv[++index], &end, 10);
            if (end == argv[index] || *end != '\0' || parsed < 0L || parsed > 232L) {
                fprintf(stderr, "Invalid DDS domain id '%s'\n", argv[index]);
                return EXIT_FAILURE;
            }
            domain_id = (int)parsed;
        } else {
            fprintf(stderr, "Unknown argument '%s'\n", argv[index]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if ((replay_path == NULL) == (interface_name == NULL)) {
        fprintf(stderr, "Choose exactly one input: --replay or --interface\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

#if !defined(J1939_DDS_HAVE_CYCLONEDDS)
    (void)domain_id;
#endif

    vehicle_telemetry_init(&gateway.telemetry, source_id);
    if (strcmp(publisher_name, "stdout") == 0) {
        status = stdout_publisher_create(&gateway.publisher);
    } else if (strcmp(publisher_name, "dds") == 0) {
#if defined(J1939_DDS_HAVE_CYCLONEDDS)
        status = cyclone_publisher_create(&gateway.publisher, domain_id);
#else
        fprintf(stderr, "DDS support is not enabled in this build\n");
        return EXIT_FAILURE;
#endif
    } else {
        fprintf(stderr, "Unknown publisher '%s'\n", publisher_name);
        return EXIT_FAILURE;
    }
    if (status != 0) {
        fprintf(stderr, "Cannot initialize publisher '%s'\n", publisher_name);
        return EXIT_FAILURE;
    }

    if (install_signal_handlers() != 0) {
        fprintf(stderr, "Cannot install signal handlers\n");
        gateway.publisher.destroy(&gateway.publisher);
        return EXIT_FAILURE;
    }

    if (replay_path != NULL) {
        status = replay_source_run(replay_path, process_message, &gateway);
    } else {
#if defined(J1939_DDS_HAVE_LINUX_J1939)
        status = linux_j1939_source_run(interface_name, process_message, &gateway);
#else
        fprintf(stderr, "Live CAN J1939 input is only available on Linux\n");
        status = -1;
#endif
    }

    gateway.publisher.destroy(&gateway.publisher);
    fprintf(
        stderr,
        "Gateway summary: decoded=%lu unsupported=%lu invalid=%lu\n",
        gateway.decoded_messages,
        gateway.unsupported_messages,
        gateway.invalid_messages);
    return status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
