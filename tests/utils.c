#include "utils.h"

uint8_t generate_next_byte(uint8_t last_byte, uint32_t order, uint32_t sed) {
    last_byte *= last_byte + 3;
    last_byte *= sed + 2;
    last_byte ^= order ^ sed;
    return last_byte;
}

