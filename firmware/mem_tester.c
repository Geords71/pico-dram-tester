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
} marchb_self.t;
static marchb_self.t marchb_self.

void init_marchb()
{
    marchb_self.phase = MARCH_PHASE_M0;
    marchb_self.descending = false;
    marchb_self.cur_addr = 0;
    marchb_self.inc = 1;
    marchb_self.failed_bits = 0;
    marchb_self.cur_bit = 0;
}

typedef struct {
    int32_t cur_addr;
    uint value;
} psrandom_self.t;
static psrandom_self.t psrandom_self.

void init_psrandom() {
    psrandom_self.cur_addr = 0;
    psrandom_self.value = 0;
}

typedef struct {
    int32_t cur_addr;
} refresh_self.t;
static refresh_self.t refresh_self.

void init_refresh() {
    refresh_self.cur_addr = 0;
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

} mem_tests_self.t;
static mem_tests_self.t self.

void init_ram_tests(mem_chip_t *mem_chip, uint8_t speed_grade, uint8_t variant)
{

    self.mem_chip = mem_chip;
    self.running = false;
    self.phase = RAM_TEST_PHASE_SLEEP;
    self.final_result = 0;

    self.mem_chip->setup_pio(speed_grade, variant);

    init_marchb();
    init_psrandom();
    init_refresh();
}

void start_ram_tests() {
    board_ram_power_on();
    self.running = true;
}

void do_ram_tests()
{
    if (!self.running) return;
}

void stop_ram_tests() {
    self.running = false;
    self.mem_chip->teardown_pio();
    board_ram_power_off();
}*/

#include <stdint.h>
#include <stdlib.h>
#include "xoroshiro64starstar.h"
#include "pico/multicore.h"
//#include "queue.h"
#include "mem_chip.h"
#include "mem_tester.h"
#include "logging/logging.h"


static mem_tester_t self;

static uint ram_bit_mask;

// Wrapper that just calls the read routine for the selected chip
static int __no_inline_not_in_flash_func(ram_read)(int addr)
{
    return self.chip->ram_read(addr);
}

// Wrapper that just calls the write routine for the selected chip
static void __no_inline_not_in_flash_func(ram_write)(int addr, int data)
{
    self.chip->ram_write(addr, data);
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
    for (i = 0; i < PSEUDO_VALUES; i++) {
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

uint32_t (*all_tests[])(void) = {
    &marchb_test,
    &psrandom_test,
    &refresh_test,
};

static const uint8_t all_tests_len = 3;

// Initial entry for the RAM test routines running
// on the second CPU core.
uint32_t __no_inline_not_in_flash_func(__run_all)()
{
    ULOG_INFO("Running all memory tests...");
    self.shared.run_state = 1;

    psrand_init_seeds();

    //uint32_t failed;

    // Initialize RAM by performing n RAS cycles
    march_element(false, 0);

    for (uint8_t i = 0; i<all_tests_len; i++) {
        self.shared.cur_test = i+1;
        self.shared.run_result = all_tests[i]();

        if (self.shared.run_result) {
            self.shared.run_state = 2;
            return self.shared.run_result;
        }
    }

    self.shared.run_state =2;
    return 0;
}

void run_all() {
    self.shared.please_run = 1;
}

void stop_all() {
    self.shared.please_run = 0;
}

const char *mem_test_names[] = {"Idle", "March-B", "Pseudo", "Refresh"};

static void reset_shared() {
    self.shared.cur_addr = 0;
    self.shared.cur_bit = 0;
    self.shared.cur_test = 0;
    self.shared.cur_subtest = 0;
    self.shared.run_state = 0;
    self.shared.run_result = 0;
    self.shared.please_run = 0;
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
            self.shared.please_run = 0;
            __run_all();
        } else {
            busy_wait_ram(100);
        };
    }
}
static void init () {
    multicore_launch_core1(core1_entry);
}

static mem_tester_t self = {
    .run_all = &run_all,
    .shared = {
        .cur_addr = 0,
        .cur_bit = 0,
        .cur_subtest = 0,
        .cur_test = 0,
        .run_state = 0,
        .run_result = 0,
        .reset = &reset_shared,
        .init = &init,
    },
};

mem_tester_t *mem_tester = &self;