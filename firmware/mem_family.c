#include "mem_family.h"
#include "stdint.h"

struct pio_program *get_patched_program(const struct pio_program *program, const uint8_t *delay_set, uint8_t delay_set_size)
{
    static struct pio_program patched_program;
    static uint16_t patched_instructions[PIO_INSTRUCTION_COUNT];

    patched_program.length = program->length;
    patched_program.origin = program->origin;
    patched_program.pio_version = program->pio_version;
    patched_program.used_gpio_ranges = program->used_gpio_ranges;

    for (uint8_t i = 0; i < program->length; i++) {
        uint16_t instruction = program->instructions[i];
        uint8_t field = (instruction >> 8) & 0x1f;

        // 0 is reserved for instructions that don't use this feature
        if ((field > 0) && (field < delay_set_size)) {
            instruction = instruction & 0xe0ff;
            instruction |= ((delay_set[field] & 0x1f) << 8);
        }
        patched_instructions[i] = instruction;
    }

    patched_program.instructions = patched_instructions;
    return &patched_program;
}