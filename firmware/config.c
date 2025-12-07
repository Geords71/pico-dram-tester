#include "config.h"
#include <stdint.h>
#include "hardware/clocks.h"
#include "hardware/vreg.h"

#define PMEMTEST_OVERCLOCK_KHZ 300000

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
