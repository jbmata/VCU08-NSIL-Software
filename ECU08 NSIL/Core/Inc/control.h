#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>

#define CTRL_TICK_FLAG (1U << 0)

void Control_Init(void);
void Control_Step(void);

#endif
