#include "j1939_dds/source.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REPLAY_LINE_LENGTH 4096U

static char *trim(char *text)
{
    char *end;

    while (*text != '\0' && isspace((unsigned char)*text) != 0) {
        ++text;
    }

    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1]) != 0) {
        --end;
    }
    *end = '\0';
    return text;
}

static int parse_unsigned(const char *text, unsigned long long maximum, unsigned long long *value)
{
    char *end = NULL;
    unsigned long long parsed;

    errno = 0;
    parsed = strtoull(text, &end, 0);
    if (errno != 0 || end == text || *trim(end) != '\0' || parsed > maximum) {
        return -1;
    }
    *value = parsed;
    return 0;
}

static int hex_nibble(char character)
{
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f') {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'F') {
        return character - 'A' + 10;
    }
    return -1;
}

static int parse_payload(char *text, j1939_message_t *message)
{
    size_t output_index = 0U;
    int high_nibble = -1;

    for (; *text != '\0'; ++text) {
        int nibble;

        if (isspace((unsigned char)*text) != 0 || *text == ':' || *text == '-') {
            continue;
        }
        nibble = hex_nibble(*text);
        if (nibble < 0) {
            return -1;
        }
        if (high_nibble < 0) {
            high_nibble = nibble;
        } else {
            if (output_index >= J1939_DDS_MAX_PAYLOAD) {
                return -1;
            }
            message->payload[output_index++] = (uint8_t)((high_nibble << 4) | nibble);
            high_nibble = -1;
        }
    }

    if (high_nibble >= 0 || output_index == 0U) {
        return -1;
    }
    message->payload_length = output_index;
    return 0;
}

static int parse_line(char *line, j1939_message_t *message)
{
    char *fields[4];
    char *cursor = line;
    unsigned long long value;
    size_t index;

    for (index = 0U; index < 3U; ++index) {
        char *comma = strchr(cursor, ',');
        if (comma == NULL) {
            return -1;
        }
        *comma = '\0';
        fields[index] = trim(cursor);
        cursor = comma + 1;
    }
    fields[3] = trim(cursor);

    memset(message, 0, sizeof(*message));
    if (parse_unsigned(fields[0], UINT64_MAX, &value) != 0) {
        return -1;
    }
    message->timestamp_ms = (uint64_t)value;

    if (parse_unsigned(fields[1], 0x3FFFFULL, &value) != 0) {
        return -1;
    }
    message->pgn = (uint32_t)value;

    if (parse_unsigned(fields[2], UINT8_MAX, &value) != 0) {
        return -1;
    }
    message->source_address = (uint8_t)value;

    return parse_payload(fields[3], message);
}

int replay_source_run(
    const char *path,
    j1939_message_callback_t callback,
    void *user_data)
{
    FILE *file;
    char line[REPLAY_LINE_LENGTH];
    unsigned long line_number = 0UL;

    if (path == NULL || callback == NULL) {
        return -1;
    }

    file = fopen(path, "r");
    if (file == NULL) {
        fprintf(stderr, "Cannot open replay file '%s': %s\n", path, strerror(errno));
        return -1;
    }

    while (fgets(line, (int)sizeof(line), file) != NULL) {
        char *content;
        j1939_message_t message;

        ++line_number;
        content = trim(line);
        if (*content == '\0' || *content == '#') {
            continue;
        }
        if (strncmp(content, "timestamp_ms", strlen("timestamp_ms")) == 0) {
            continue;
        }
        if (parse_line(content, &message) != 0) {
            fprintf(stderr, "Invalid replay record at %s:%lu\n", path, line_number);
            (void)fclose(file);
            return -1;
        }
        if (callback(&message, user_data) != 0) {
            break;
        }
    }

    if (ferror(file) != 0) {
        fprintf(stderr, "Error while reading replay file '%s'\n", path);
        (void)fclose(file);
        return -1;
    }

    return fclose(file) == 0 ? 0 : -1;
}
