#ifndef __hw_h__
#define __hw_h__

#include <stdint.h>

int init_hw();

void set_color(uint8_t day, uint8_t r, uint8_t w, uint8_t c1, uint8_t c2);

void close_hw();

#endif

