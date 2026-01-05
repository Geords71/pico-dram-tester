#include "config.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ff.h>
#include "logging/logging.h"
#include "shared_storage/shared_storage.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"

#define PMEMTEST_OVERCLOCK_KHZ 300000
#define CONFIG_FILE_PATH "SYSTEM.CFG"

uint32_t get_system_overclock()
{
    return clock_get_hz(clk_sys);
}

void set_system_overclock()
{
    // Increase core voltage slightly (default is 1.10V) to better handle overclock
    vreg_set_voltage(VREG_VOLTAGE_1_20);

    // Overclock! It should panic if it can't reach this as we have set second argument to true
    set_sys_clock_khz(PMEMTEST_OVERCLOCK_KHZ, true);
}

void do_system_config()
{
    set_system_overclock();
};

static bool parse_int32(const char* key, const char* value, void* out) {
    char* end;
    long v = strtol(value, &end, 10);

    // No digits parsed
    if (end == value) return false;

    // Trailing garbage
    if (*end != '\0') return false;

    // Overflow or underflow relative to long
    if (errno == ERANGE) return false;

    // Now clamp/check for int32_t range explicitly
    if (v < INT32_MIN || v > INT32_MAX) return false;

    *(int32_t*)out = (int32_t)v;
    int32_t *p = out;

    ULOG_INFO("app_config.%s has been set to %d.", key, *p);
    return true;
}

static bool parse_bool(const char* key, const char* value, void* out) {
    bool* p = (bool*)out;

    bool good_val = false;

    if (strcasecmp(value, "true") == 0 || strcmp(value, "1") == 0) {
        *p = true;
        good_val = true;
    }
    if (strcasecmp(value, "false") == 0 || strcmp(value, "0") == 0) {
        *p = false;
        good_val = true;
    }

    if (good_val) 
    {
        ULOG_INFO("app_config.%s has been set to %s.", key, *p ? true : false);
    }

    return good_val;
}

typedef bool (*parse_fn_t)(const char* key, const char* value, void* out);

typedef struct {
    const char* key;
    size_t offset;
    parse_fn_t parse;
} config_field_t;

app_config_t app_config = {
    .led_on = true,
    .enc_debounce_count = 1000
};

static const config_field_t config_schema[] = {
    { "led_on",             offsetof(app_config_t, led_on),             parse_bool},
    { "enc_debounce_count", offsetof(app_config_t, enc_debounce_count), parse_int32},
};

void parse_config_line(const char* key, const char* value) {

    for (size_t i = 0; i < sizeof(config_schema)/sizeof(config_schema[0]); i++)
    {
        if (strcmp(key, config_schema[i].key) == 0)
        {
            void* field_ptr = &app_config + config_schema[i].offset;
            static bool retval;

            retval = config_schema[i].parse(key, value, field_ptr);
            if (!retval)
            {
                ULOG_WARNING("Unknown or invalid config key: %s", key);
            }
        }
    }
}


void load_app_config() {
    ULOG_INFO("Loading settings from %s...", CONFIG_FILE_PATH);
    if (mount_shared_storage() != FR_OK)
    {
        ULOG_INFO("Can't mount Filesystem. Using hard-coded defaults.");
        return;
    }

    FIL fp;
    bool result = f_open(&fp, CONFIG_FILE_PATH, FA_READ);

    if (result != FR_OK)
    {
        ULOG_WARNING("Can't open %s. Using hard-coded defaults: %d", CONFIG_FILE_PATH, result);
        return;
    }

    char key[64], value[256], line[128];

    while (f_gets(line, sizeof(line), &fp))
    {
        if (sscanf(line, "%63[^=]=%255s", key, value) == 2)
        {
            parse_config_line(key, value);
        }
    }

    f_close(&fp);
    unmount_shared_storage();
}