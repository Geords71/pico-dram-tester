#include <stdint.h>
#include <stdlib.h>
#include "xoroshiro64starstar.h"
#include "pico/multicore.h"
#include "config.h"
#include "mem_chip.h"
#include "mem_tester.h"
#include "logging/logging.h"

static mem_tester_t self;
static uint ram_bit_mask;

// Wrapper that just calls the read routine for the selected chip
static int __no_inline_not_in_flash_func(ram_read)(int addr)
{
    return self.chip->get_family()->read(
        self.chip->variants.list[self.variant_idx].addr_func,
        addr
    );
}

// Wrapper that just calls the write routine for the selected chip
static void __no_inline_not_in_flash_func(ram_write)(int addr, int data)
{
    self.chip->get_family()->write(
        self.chip->variants.list[self.variant_idx].addr_func,
        addr,
        data
    );
}

// Low level routines for march-b algorithm
static bool __no_inline_not_in_flash_func(me_r0)(int a)
{
    int bit = ram_read(a) & ram_bit_mask;
    return (bit == 0);
}

static bool  __no_inline_not_in_flash_func(me_r1)(int a)
{
    int bit = ram_read(a) & ram_bit_mask;
    return (bit == ram_bit_mask);
}

static bool __no_inline_not_in_flash_func(me_w0)(int a)
{
    ram_write(a, ~ram_bit_mask);
    return true;
}

static bool __no_inline_not_in_flash_func(me_w1)(int a)
{
    ram_write(a, ram_bit_mask);
    return true;
}

static bool __no_inline_not_in_flash_func(marchb_m0)(int a)
{
    me_w0(a);
    return true;
}

static bool __no_inline_not_in_flash_func(marchb_m1)(int a)
{
    return me_r0(a) && me_w1(a) && me_r1(a) && me_w0(a) && me_r0(a) && me_w1(a);
}

static bool __no_inline_not_in_flash_func(marchb_m2)(int a)
{
    return me_r1(a) && me_w0(a) && me_w1(a);
}

static bool __no_inline_not_in_flash_func(marchb_m3)(int a)
{
    return me_r1(a) && me_w0(a) && me_w1(a) && me_w0(a);
}

static bool __no_inline_not_in_flash_func(marchb_m4)(int a)
{
    return me_r0(a) && me_w1(a) && me_w0(a);
}

static bool __no_inline_not_in_flash_func(march_element)(bool descending, int algorithm)
{
    int inc = descending ? -1 : 1;
    int start = descending ? (self.chip->mem_size - 1) : 0;
    int end = descending ? -1 : self.chip->mem_size;
    int a;
    bool ret;

    self.shared.cur_subtest = algorithm;

    for (self.shared.cur_addr = start; self.shared.cur_addr != end; self.shared.cur_addr += inc) {
        switch (algorithm) {
            case 0:
                ret = marchb_m0(self.shared.cur_addr);
                break;
            case 1:
                ret = marchb_m1(self.shared.cur_addr);
                break;
            case 2:
                ret = marchb_m2(self.shared.cur_addr);
                break;
            case 3:
                ret = marchb_m3(self.shared.cur_addr);
                break;
            case 4:
                ret = marchb_m4(self.shared.cur_addr);
                break;
            default:
                break;
        }
        if (!ret) {
            ULOG_INFO("MarchB M%d test failed at 0x%05X", algorithm, self.shared.cur_addr);
            return false;
        }
    }
    return true;
}

uint32_t __no_inline_not_in_flash_func(marchb_testbit)()
{
    bool ret;
    ret = march_element(false, 0);
    if (!ret) return false;
    ret = march_element(false, 1);
    if (!ret) return false;
    ret = march_element(false, 2);
    if (!ret) return false;
    ret = march_element(true, 3);
    if (!ret) return false;
    ret = march_element(true, 4);
    if (!ret) return false;
    return true;
}

// Runs the memory test on the 2nd core
uint32_t __no_inline_not_in_flash_func(marchb_test)()
{
    ULOG_INFO("MarchB test started...");
    int failed = 0;
    int bit = 0;

    for (bit = 0; bit < self.chip->bits; bit++) {
        self.shared.cur_bit = bit;
        ram_bit_mask = 1 << bit;
        if (!marchb_testbit(self.chip->mem_size)) {
            failed |= 1 << bit; // fail flag
        }
    }

    if (failed == 0) {
        ULOG_INFO("MarchB test passed.");
    } else {
        ULOG_INFO("MarchB test failed.");
    }
    return (uint32_t)failed;
}

#define MAX_PSEUDO_VALUES 64
#define ARTISANAL_NUMBER 42
static uint64_t random_seeds[MAX_PSEUDO_VALUES];
static uint32_t pseudo_values;

void __no_inline_not_in_flash_func(psrand_init_seeds)()
{
    config_t *cfg = config(false);
    pseudo_values = cfg->tests_pseudo_values;
    pseudo_values = (pseudo_values < MAX_PSEUDO_VALUES) ? pseudo_values : MAX_PSEUDO_VALUES;

    ULOG_INFO("Pseudorandom seeds initialized with %d values...", pseudo_values);

    psrand_seed(ARTISANAL_NUMBER);
    for (int i = 0; i < pseudo_values; i++) {
        random_seeds[i] = psrand_next();
    }
}

uint32_t __no_inline_not_in_flash_func(psrand_next_bits)()
{
    static int bitcount = 0;
    static uint32_t cur_rand;
    uint32_t out;

    if (bitcount < self.chip->bits) {
        cur_rand = psrand_next();
        bitcount = 32;
    }

    out = cur_rand & ((1 << (self.chip->bits)) - 1);
    cur_rand = cur_rand >> self.chip->bits;
    bitcount -= self.chip->bits;
    return out;
}


// Pseudorandom test
uint32_t __no_inline_not_in_flash_func(psrandom_test)()
{
    ULOG_INFO("Pseudorandom test started...");
    uint i;
    uint32_t bitsout;
    uint32_t bitsin;

    // Write seeded pseudorandom data
    for (i = 0; i < pseudo_values; i++) {
        self.shared.cur_subtest = i >> 2;
        self.shared.cur_bit = i & 3;
        psrand_seed(random_seeds[i]);
        for (self.shared.cur_addr = 0; self.shared.cur_addr < self.chip->mem_size; self.shared.cur_addr++) {
            bitsout = psrand_next_bits(self.chip->bits);
            ram_write(self.shared.cur_addr, bitsout);
        }

        // Reseed and then read the data back
        psrand_seed(random_seeds[i]);
        for (self.shared.cur_addr = 0; self.shared.cur_addr < self.chip->mem_size; self.shared.cur_addr++) {
            bitsout = psrand_next_bits(self.chip->bits);
            bitsin = ram_read(self.shared.cur_addr);
            if (bitsout != bitsin) {
                ULOG_INFO("Pseudorandom test failed. Expected %d but got %d at 0x%05X",
                    bitsout, bitsin, self.shared.cur_addr);
                return 1;
            }
        }
    }

    ULOG_INFO("Pseudorandom test passed.");
    return 0;
}

uint32_t get_refresh_bitsout(int addr, uint32_t bits)
{
    static uint32_t bitmask;
    bitmask = (1 << bits) - 1; 
    return (addr & bitmask);
}

uint32_t __no_inline_not_in_flash_func(refresh_test)()
{
    ULOG_INFO("Refresh test started...");
    static uint32_t bitsout;
    static uint32_t bitsin;

    // Create a bit mask of ones that that is chip->bits wide. 
    // This will mean that we get a stream  0-1 values for 1 bit
    // test and 0-F values for four bit test - based on the low bits
    // of the current address.
    for (self.shared.cur_addr = 0; self.shared.cur_addr < self.chip->mem_size; self.shared.cur_addr++) {
        bitsout = get_refresh_bitsout(self.shared.cur_addr, self.chip->bits);
        ram_write(self.shared.cur_addr, bitsout);
    }

    sleep_us(5000);

    for (self.shared.cur_addr = 0; self.shared.cur_addr < self.chip->mem_size; self.shared.cur_addr++) {
        bitsout = get_refresh_bitsout(self.shared.cur_addr, self.chip->bits);
        bitsin = ram_read(self.shared.cur_addr);

        if (bitsout != bitsin) {
            ULOG_INFO("Refresh test failed: expected %d, but got %d at 0x%05X",
                 bitsout, bitsin, self.shared.cur_addr);
            return 1;
        }
    }
    ULOG_INFO("Refresh test passed.");
    return 0;
}

static uint32_t warmup_test () {
    march_element(false, 0);
    return 0;
}

enum mem_test_names_t {
    MEM_TEST_FUNC_WARMUP,
    MEM_TEST_FUNC_MARCHB,
    MEM_TEST_FUNC_PSEUDO,
    MEM_TEST_FUNC_REFRESH,
    MEM_TEST_FUNC_COUNT,
};

const char *mem_test_names[] = {
    [MEM_TEST_FUNC_WARMUP] = "Warm-Up",
    [MEM_TEST_FUNC_MARCHB] = "March-B",
    [MEM_TEST_FUNC_PSEUDO] = "Pseudo",
    [MEM_TEST_FUNC_REFRESH] = "Refresh",
};

uint32_t (*all_tests[])(void) = {
    [MEM_TEST_FUNC_WARMUP] = &warmup_test,
    [MEM_TEST_FUNC_MARCHB] = &marchb_test,
    [MEM_TEST_FUNC_PSEUDO] = &psrandom_test,
    [MEM_TEST_FUNC_REFRESH] = &refresh_test,
};

// Initial entry for the RAM test routines running
// on the second CPU core.
uint32_t __no_inline_not_in_flash_func(__run_all)()
{

    psrand_init_seeds();

    for (uint8_t i = 0; i<MEM_TEST_FUNC_COUNT; i++) {
        self.shared.cur_test = i;
        self.shared.run_result = all_tests[i]();

        if (self.shared.run_result) {
            return self.shared.run_result;
        }
    }

    return 0;
}

void run_all() {
    self.shared.please_run = 1;
}

void stop_all() {
    self.shared.please_run = 0;
}

// Reset the shared variables that should not persist between runs
static void reset_shared() {
    self.shared.cur_addr = 0;
    self.shared.cur_bit = 0;
    self.shared.cur_test = MEM_TEST_FUNC_WARMUP;
    self.shared.cur_subtest = 0;
    self.shared.run_state = MEM_TESTER_IDLE;
    self.shared.run_result = 0;
    self.shared.please_run = false;
    self.shared.please_stop = false;
}


static void __no_inline_not_in_flash_func(busy_wait_ram)(uint32_t ms) {
    // Each loop is roughly 10-20 cycles. 
    // At 150MHz (Pico 2), this is a "human perceivable" delay.
    for (volatile uint32_t i = 0; i < ms * 10000; i++) {
        __asm("nop");
    }
}

// Entry point for second core. This is just a generic
// function dispatcher lifted from the Raspberry Pi example code.
static void __no_inline_not_in_flash_func(core1_entry)() {
    while (true) {

        if (self.shared.please_run)
        {
            // Re-read config to get any changes to test settings.
            config_t *cfg = config(true);

            ULOG_INFO("Setting up PIO...");
            board_ram_power_on();
            mem_chip_load_config(self.chip);

            const uint8_t *delay_set = self.chip->delay_sets.list[self.speed_idx];
            self.chip->get_family()->setup_pio(delay_set);

            ULOG_INFO("Testing %s chip at %s...", self.chip->name, self.chip->delay_sets.names[self.speed_idx]);
            ULOG_INFO("Running all memory tests...");
            self.shared.run_state = MEM_TESTER_RUNNING;

            uint32_t return_code = __run_all();
            ULOG_INFO("Tests finished with code:%d, soak:%d, stop:%d...", return_code, self.shared.please_soak, self.shared.please_stop);

            ULOG_INFO("Tearing down PIO...");
            self.chip->get_family()->teardown_pio();
            board_ram_power_off();

            // If we've selected soak mode, the tests are passing, and the
            // user hasn't requested we stop, keep going!
            if (return_code == 0 && self.shared.please_soak && !self.shared.please_stop) {
                ULOG_INFO("Re-Running all memory tests...");
                continue;
            } ;
            
            if (return_code != 0 && self.shared.please_seek && self.speed_idx < self.chip->delay_sets.len - 1) {
                ULOG_INFO("Re-Running all memory tests at a slower speed...");
                self.speed_idx++;
                continue;
            };

            // We either failed, were only doing a single run, or the
            // user asked us to stop.
            ULOG_INFO("Memory test finished...");
            self.shared.please_run = 0;
            self.shared.run_state = MEM_TESTER_FINISHED;
        } else {
            busy_wait_ram(100);
        };
    }
}

static void init () {
    multicore_launch_core1(core1_entry);
}

static void sleep() {
    multicore_reset_core1();
}

static mem_tester_t self = {
    .run_all = &run_all,
    .shared = {
        .cur_addr = 0,
        .cur_bit = 0,
        .cur_subtest = 0,
        .cur_test = 0,
        .run_state = MEM_TESTER_IDLE,
        .run_result = 0,
        .reset = &reset_shared,
        .init = &init,
        .sleep = &sleep,
    },
};

mem_tester_t *mem_tester = &self;