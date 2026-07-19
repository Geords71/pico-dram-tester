#ifndef _MEM_TESTS_H
#define _MEM_TESTS_H

/*
extern void init_ram_tests();
extern void do_ram_tests();
*/

#include <stdatomic.h>
#include <stdint.h>
#include "mem_chip.h"

typedef struct {
    _Atomic uint32_t cur_addr;
    _Atomic uint32_t cur_bit;
    _Atomic uint32_t cur_test;
    _Atomic uint32_t cur_subtest;
    _Atomic uint32_t run_state;
    _Atomic uint32_t run_result;
    _Atomic uint32_t please_run;
    _Atomic uint32_t please_stop;
    _Atomic uint32_t please_soak;
    void (* init)();
    void (* reset)();

} mem_tester_shared_state_t;

typedef struct {
    void (*run_all)();
    const mem_chip_t *chip;                                
    uint16_t variant_idx;
    uint16_t speed_idx;
    mem_tester_shared_state_t shared;
} mem_tester_t;

extern mem_tester_t *mem_tester;

extern const char *mem_test_names[];

enum mem_tester_run_state_t {
    MEM_TESTER_IDLE,
    MEM_TESTER_RUNNING,
    MEM_TESTER_FINISHED,
    MEM_TESTER_RUN_STATES_COUNT,
};

#endif