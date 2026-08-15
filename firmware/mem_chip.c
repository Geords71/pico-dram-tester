#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mem_chip.h"
#include "hardware/pio.h"
#include "ff.h"
#include "shared_storage.h"
#include "logging.h"

PIO pio;
uint sm = 0;
uint offset; // Returns offset of starting instruction

void mem_chip_load_config(mem_chip_t *chip) {

    if (mount_shared_storage() != FR_OK) {
        ULOG_WARNING("Shared USB storage could not be mounted. Hard coded delay values will be used.");
        return;
    }

    char filename[32];
    int filename_len = snprintf(filename, sizeof(filename), "%s.csv", chip->timing_family);
    if (filename_len < 0 || filename_len >= sizeof(filename)) {
        ULOG_ERROR("Timing filename is too long: %s", chip->timing_family);
        unmount_shared_storage();
        return;
    }

    FIL fp;
    bool result = f_open(&fp, filename, FA_READ);

    if (result == FR_OK) {
        ULOG_INFO("Reading delay values from %s...", filename);
        uint8_t buffer[512] = {"\0"};

        for (uint8_t row=0; row<chip->delay_sets.len; row++) {

            if (f_gets(buffer, sizeof(buffer), &fp) == NULL) {
                ULOG_INFO("Reached delay file EOF.");
                break;
            }
            buffer[strcspn(buffer, "\n")] = 0;

            ULOG_INFO("    Patching delay line %d: %s", row, buffer);

            char *token;
            token = strtok(buffer, ",");
            for (uint8_t col=0; col<chip->delay_sets.wid; col++) {
                if (token == NULL) {
                    ULOG_ERROR("Encountered unexpected end of line. Continuing to next...");
                    continue;
                }
                chip->delay_sets.list[row][col] = atoi(token);
                token = strtok(NULL, ",");
            }
        }

        f_close(&fp);
        result = true;

    } else {
        ULOG_WARNING("Can't open %s. Using hard-coded delay values: %d", filename, result);
        result = false;
    }
    unmount_shared_storage();
}