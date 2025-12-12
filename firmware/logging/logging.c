#include <stdio.h>
#include <stdint.h>
#include <pico/stdlib.h>
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

void console_logger(ulog_level_t severity, char *msg) {
     printf(
        "%s [%s]: %s\n",
         get_timestamp(),
         ulog_level_name(severity),
         msg);
}

void init_logging()
{
    ULOG_INIT();
    ULOG_SUBSCRIBE(console_logger, ULOG_INFO_LEVEL);
}