#ifndef __hw_h__
#define __hw_h__

#include <stdint.h>

int init_hw(int handle);
void close_hw(int handle);

void set_color(uint8_t day, uint8_t r, uint8_t w, uint8_t c1, uint8_t c2, uint8_t maxtime);

int button_alive();
int is_button_request();
void send_button_request();

int is_reset_signal();
void send_beep(uint8_t n);

#endif

