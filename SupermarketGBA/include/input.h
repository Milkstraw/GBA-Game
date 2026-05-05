#ifndef INPUT_H
#define INPUT_H

#include "gba_types.h"

extern u16 keys_held;
extern u16 keys_pressed;
extern u16 keys_released;

void input_poll(void);

#define KEY_HELD(k)      ((keys_held     & (k)) != 0)
#define KEY_PRESSED(k)   ((keys_pressed  & (k)) != 0)
#define KEY_RELEASED(k)  ((keys_released & (k)) != 0)

#endif /* INPUT_H */
