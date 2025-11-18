#include <picotest.h>

#include "hooks.h"

PICOTEST_SUITE(main_suite, test_something);

#define PICOTEST_MAINSUITE main_suite
#include <picotestRunner.inc>