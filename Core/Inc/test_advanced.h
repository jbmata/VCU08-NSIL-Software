#ifndef TEST_ADVANCED_H
#define TEST_ADVANCED_H

#include <stdint.h>

/* Advanced Test Suite - ECU08 NSIL */

typedef struct {
  uint16_t total;
  uint16_t passed;
  uint16_t failed;
  uint32_t duration_ms;
} TestStats_t;

/* Public API */
TestStats_t Test_AdvancedRunAll(void);
TestStats_t Test_StressCycling(uint32_t cycles);
TestStats_t Test_BoundaryConditions(void);
TestStats_t Test_ConcurrencySimulation(void);
TestStats_t Test_CANMessageExchange(void);

#endif /* TEST_ADVANCED_H */
