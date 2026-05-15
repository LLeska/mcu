#include "mem-task.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

void mem(uint32_t addr) {
    if (addr % 4 != 0) {
        printf("Error: Address 0x%08X is not aligned! Must be multiple of 4.\n", addr);
        return;
    }
    uint32_t value = *(volatile uint32_t*)addr;
    printf("value 0x%08X\n", value);
}
void wmem(uint32_t addr, uint32_t value) {
    *(volatile uint32_t*)addr = value;
}