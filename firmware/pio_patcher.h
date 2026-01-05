#ifndef _PIO_PATCHER_H
#define _PIO_PATCHER_H

void set_current_pio_program(const struct pio_program *prog);
struct pio_program *get_current_pio_program();
void pio_patch_delays(const uint8_t *delays, uint8_t length);

#endif
