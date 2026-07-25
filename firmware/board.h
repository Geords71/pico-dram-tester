#ifndef _BOARD_H
#define _BOARD_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    clockwise,
    anticlockwise,

} board_encoder_movement;

extern void board_pre_init();
extern void board_init();
extern void board_ram_power_on();
extern void board_ram_power_off();

extern bool board_encoder_pushed();
extern bool board_back_pushed();

#define BOARD_ENCODER_ROTATION_NONE 0
#define BOARD_ENCODER_ROTATION_CLOCKWISE 1
#define BOARD_ENCODER_ROTATION_ANTICLOCKWISE 2
extern uint8_t do_board_encoder();
#endif