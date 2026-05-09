#ifndef _MEM_TESTS_H
#define _MEM_TESTS_H

/*
extern void init_ram_tests();
extern void do_ram_tests();
*/

extern uint32_t all_ram_tests(const mem_chip_t *);

extern volatile int stat_cur_addr;
extern volatile int stat_old_addr;
extern volatile int stat_cur_bit;
extern volatile int stat_cur_subtest;
extern void psrand_init_seeds();

#endif