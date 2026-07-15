#include "config.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ff.h>
#include "logging/logging.h"
#include "shared_storage/shared_storage.h"

#define CONFIG_FILE_PATH "SYSTEM.CFG"

//uint32_t get_system_overclock()
//{
//    return clock_get_hz(clk_sys);
//}



static bool parse_int32(const char* key, const char* value, void* out) {
    int32_t* p = (int32_t*)out;
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

    *p = v;

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
        ULOG_INFO("app_config.%s has been set to %s.", key, (*p ? "true" : "false"));
    }

    return good_val;
}

typedef bool (*parse_fn_t)(const char* key, const char* value, void* out);

typedef struct {
    const char* key;
    size_t offset;
    parse_fn_t parse;
} config_field_t;

static config_t self = {
    .led_on = true,
    .enc_states_per_click = 2,
};

static const config_field_t config_schema[] = {
    {"led_on",                     offsetof(config_t, led_on),                     parse_bool},
    {"enc_states_per_click",       offsetof(config_t, enc_states_per_click),       parse_int32},
};

static void parse_config_line(const char* key, const char* value) {

    for (size_t i = 0; i < sizeof(config_schema)/sizeof(config_schema[0]); i++)
    {
        if (strcmp(key, config_schema[i].key) == 0)
        {
            void* field_ptr = (char*)&self + config_schema[i].offset;
            static bool retval;

            retval = config_schema[i].parse(key, value, field_ptr);
            if (!retval)
            {
                ULOG_WARNING("Unknown or invalid config key: %s", key);
            }
        }
    }
}

static bool loaded = false;

static void load() {
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
    loaded = true;
}

config_t * config() {
    if (!loaded) load();
    return &self;
}