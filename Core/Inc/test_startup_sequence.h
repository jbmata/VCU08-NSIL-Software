#ifndef TEST_STARTUP_SEQUENCE_H
#define TEST_STARTUP_SEQUENCE_H

#include <stdint.h>

/* Startup Sequence Test Suite */

typedef struct {
  uint16_t total;
  uint16_t passed;
  uint16_t failed;
} TestStats_Startup_t;

/* Run all 9 startup sequence tests */
TestStats_Startup_t Test_StartupSequenceRunAll(void);

#endif /* TEST_STARTUP_SEQUENCE_H */
