#include <stdio.h>
#include <stdint.h>
#include <pico/stdlib.h>
#include <tusb.h>
#include "logging.h"
#include "ulog.h"

char* get_timestamp()
{
    static char timestamp[12];

    uint64_t total_us = time_us_64();
    uint64_t seconds = total_us / 1000000;
    uint64_t us = total_us % 1000000;

    int days    = seconds / 86400;             // 86400 seconds in a day
    int hours   = (seconds % 86400) / 3600;    // remainder hours
    int minutes = (seconds % 3600) / 60;       // remainder minutes
    int secs    = seconds % 60;                // remainder seconds

    sprintf(timestamp, "%03d:%02d:%02d:%02d.%06llu", days, hours, minutes, secs, us);

    return timestamp;
}

#define LOG_BUFFER_SIZE 64  // Max number of entries
#define LOG_ENTRY_SIZE  128 // Max chars per entry
 
static char log_buffer[LOG_BUFFER_SIZE][LOG_ENTRY_SIZE];
static int head = 0, tail = 0;
int grace_period = 10;

void console_logger(ulog_level_t severity, char *msg) {

    static char log_line[LOG_ENTRY_SIZE];
    sprintf(
        log_line,
        "%s [%s]: %s\n",
        get_timestamp(),
        ulog_level_name(severity),
        msg
    );

    if (grace_period == 0)
    {
        printf(log_line);
    }
    else
    {
        // Queue into ring buffer
        strncpy(log_buffer[head], log_line, LOG_ENTRY_SIZE - 1);
        log_buffer[head][LOG_ENTRY_SIZE - 1] = '\0';
        head = (head + 1) % LOG_BUFFER_SIZE;
        if (head == tail) {
            // overwrite oldest
            tail = (tail + 1) % LOG_BUFFER_SIZE;
        }
    }
}

void flush_logging()
{
    if (stdio_usb_connected() && grace_period !=0)
    {
        // Fake a log line to kick serial montor into action. VSCode needs to be
        // fed a few lines to kick it into action. No amount of waiting alone
        // worked for me. But send a few lines and it gets things moving.
        printf("000:00:00:00.000000 [INFO]: Flushing Log Buffer: %d\n", grace_period);
        grace_period--;
    }

    if (grace_period == 0) {
        while (tail != head) {
            printf("%s", log_buffer[tail]);
            tail = (tail + 1) % LOG_BUFFER_SIZE;
        }

    }
}

void init_logging()
{
    ULOG_INIT();
    ULOG_SUBSCRIBE(console_logger, ULOG_INFO_LEVEL);
}