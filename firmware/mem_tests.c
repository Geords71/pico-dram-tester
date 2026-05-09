/*#include "pico/types.h"
#include "mem_chip.h"
#include "board.h"

typedef enum {
    MARCH_PHASE_M0 = 0,
    MARCH_PHASE_M1,
    MARCH_PHASE_M2,
    MARCH_PHASE_M3,
    MARCH_PHASE_M4,
    MARCH_PHASE_DONE
} march_phase_t;

typedef struct {
    march_phase_t phase;
    bool descending;
    int32_t cur_addr;
    int32_t inc;
    uint32_t failed_bits;   // bitmask of failed bit tests
    uint32_t cur_bit;
} marchb_ctx_t;
static marchb_ctx_t marchb_ctx;

void init_marchb()
{
    marchb_ctx.phase = MARCH_PHASE_M0;
    marchb_ctx.descending = false;
    marchb_ctx.cur_addr = 0;
    marchb_ctx.inc = 1;
    marchb_ctx.failed_bits = 0;
    marchb_ctx.cur_bit = 0;
}

typedef struct {
    int32_t cur_addr;
    uint value;
} psrandom_ctx_t;
static psrandom_ctx_t psrandom_ctx;

void init_psrandom() {
    psrandom_ctx.cur_addr = 0;
    psrandom_ctx.value = 0;
}

typedef struct {
    int32_t cur_addr;
} refresh_ctx_t;
static refresh_ctx_t refresh_ctx;

void init_refresh() {
    refresh_ctx.cur_addr = 0;
}
    
typedef enum {
    RAM_TEST_PHASE_SLEEP = 0,
    RAM_TEST_PHASE_INIT,
    RAM_TEST_PHASE_MARCHB,
    RAM_TEST_PHASE_PSEUDO,
    RAM_TEST_PHASE_REFRESH,
    RAM_TEST_PHASE_DONE,
} ram_test_phase_t;

typedef struct {
    mem_chip_t *mem_chip;                                
    ram_test_phase_t phase; 
    uint32_t final_result;
    bool running;

} mem_tests_ctx_t;
static mem_tests_ctx_t ctx;

void init_ram_tests(mem_chip_t *mem_chip, uint8_t speed_grade, uint8_t variant)
{

    ctx.mem_chip = mem_chip;
    ctx.running = false;
    ctx.phase = RAM_TEST_PHASE_SLEEP;
    ctx.final_result = 0;

    ctx.mem_chip->setup_pio(speed_grade, variant);

    init_marchb();
    init_psrandom();
    init_refresh();
}

void start_ram_tests() {
    board_ram_power_on();
    ctx.running = true;
}

void do_ram_tests()
{
    if (!ctx.running) return;
}

void stop_ram_tests() {
    ctx.running = false;
    ctx.mem_chip->teardown_pio();
    board_ram_power_off();
}*/

#include <stdint.h>
#include <stdlib.h>
#include "xoroshiro64starstar.h"
#include "queue.h"
#include "mem_chip.h"
#include "logging/logging.h"

typedef struct {
    const mem_chip_t *chip;                                
//    ram_test_phase_t phase; 
 //   uint32_t final_result;
 //   bool running;

} mem_tests_ctx_t;
static mem_tests_ctx_t ctx;

// Status shared variables between cores. Not really thread safe but this
// status is unimportant.
volatile int stat_cur_addr;
volatile int stat_old_addr;
volatile int stat_cur_bit;
volatile int stat_cur_subtest;

static uint ram_bit_mask;

// Wrapper that just calls the read routine for the selected chip
static int __no_inline_not_in_flash_func(ram_read)(int addr)
{
    return ctx.chip->ram_read(addr);
}

// Wrapper that just calls the write routine for the selected chip
static void __no_inline_not_in_flash_func(ram_write)(int addr, int data)
{
    ctx.chip->ram_write(addr, data);
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
    int start = descending ? (ctx.chip->mem_size - 1) : 0;
    int end = descending ? -1 : ctx.chip->mem_size;
    int a;
    bool ret;

    stat_cur_subtest = algorithm;

    for (stat_cur_addr = start; stat_cur_addr != end; stat_cur_addr += inc) {
        switch (algorithm) {
            case 0:
                ret = marchb_m0(stat_cur_addr);
                break;
            case 1:
                ret = marchb_m1(stat_cur_addr);
                break;
            case 2:
                ret = marchb_m2(stat_cur_addr);
                break;
            case 3:
                ret = marchb_m3(stat_cur_addr);
                break;
            case 4:
                ret = marchb_m4(stat_cur_addr);
                break;
            default:
                break;
        }
        if (!ret) {
            ULOG_INFO("MarchB M%d test failed at 0x%05X", algorithm, stat_cur_addr);
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

    for (bit = 0; bit < ctx.chip->bits; bit++) {
        stat_cur_bit = bit;
        ram_bit_mask = 1 << bit;
        if (!marchb_testbit(ctx.chip->mem_size)) {
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

#define PSEUDO_VALUES 64
#define ARTISANAL_NUMBER 42
static uint64_t random_seeds[PSEUDO_VALUES];

void __no_inline_not_in_flash_func(psrand_init_seeds)()
{
    int i;
    psrand_seed(ARTISANAL_NUMBER);
    for (i = 0; i < PSEUDO_VALUES; i++) {
        random_seeds[i] = psrand_next();
    }
}

uint32_t __no_inline_not_in_flash_func(psrand_next_bits)()
{
    static int bitcount = 0;
    static uint32_t cur_rand;
    uint32_t out;

    if (bitcount < ctx.chip->bits) {
        cur_rand = psrand_next();
        bitcount = 32;
    }

    out = cur_rand & ((1 << (ctx.chip->bits)) - 1);
    cur_rand = cur_rand >> ctx.chip->bits;
    bitcount -= ctx.chip->bits;
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
    for (i = 0; i < PSEUDO_VALUES; i++) {
        stat_cur_subtest = i >> 2;
        stat_cur_bit = i & 3;
        psrand_seed(random_seeds[i]);
        for (stat_cur_addr = 0; stat_cur_addr < ctx.chip->mem_size; stat_cur_addr++) {
            bitsout = psrand_next_bits(ctx.chip->bits);
            ram_write(stat_cur_addr, bitsout);
        }

        // Reseed and then read the data back
        psrand_seed(random_seeds[i]);
        for (stat_cur_addr = 0; stat_cur_addr < ctx.chip->mem_size; stat_cur_addr++) {
            bitsout = psrand_next_bits(ctx.chip->bits);
            bitsin = ram_read(stat_cur_addr);
            if (bitsout != bitsin) {
                ULOG_INFO("Pseudorandom test failed. Expected %d but got %d at 0x%05X",
                    bitsout, bitsin, stat_cur_addr);
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
    for (stat_cur_addr = 0; stat_cur_addr < ctx.chip->mem_size; stat_cur_addr++) {
        bitsout = get_refresh_bitsout(stat_cur_addr, ctx.chip->bits);
        ram_write(stat_cur_addr, bitsout);
    }

    sleep_us(5000);

    for (stat_cur_addr = 0; stat_cur_addr < ctx.chip->mem_size; stat_cur_addr++) {
        bitsout = get_refresh_bitsout(stat_cur_addr, ctx.chip->bits);
        bitsin = ram_read(stat_cur_addr);

        if (bitsout != bitsin) {
            ULOG_INFO("Refresh test failed: expected %d, but got %d at 0x%05X",
                 bitsout, bitsin, stat_cur_addr);
            return 1;
        }
    }
    ULOG_INFO("Refresh test passed.");
    return 0;
}

// Initial entry for the RAM test routines running
// on the second CPU core.
uint32_t __no_inline_not_in_flash_func(all_ram_tests)(const mem_chip_t *chip)
{
    psrand_init_seeds();

    ctx.chip = chip;

    int failed;
    int test = 0;

    // Initialize RAM by performing n RAS cycles
    march_element(false, 0);

    // Now run actual tests
    queue_add_blocking(&stat_cur_test, &test);
    failed = marchb_test();
    if (failed) return failed;
    test = 1;

    queue_add_blocking(&stat_cur_test, &test);
    failed = psrandom_test();
    if (failed) return failed;
    test = 2;

    queue_add_blocking(&stat_cur_test, &test);
    failed = refresh_test();
    if (failed) return failed;
    return 0;
}
