/**
 ******************************************************************************
 * @file           : test_integration.h
 * @brief          : Full integration tests for ECU08 NSIL
 *                   Tests with all FreeRTOS tasks running simultaneously
 ******************************************************************************
 */

#ifndef TEST_INTEGRATION_H
#define TEST_INTEGRATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "app_state.h"
#include "control.h"
#include "can.h"

/* Test result structure */
typedef struct {
  uint32_t total;
  uint32_t passed;
  uint32_t failed;
  uint32_t execution_time_ms;
} test_result_t;

/**
 * @brief Run ALL integration tests
 * @return test_result_t with statistics
 */
test_result_t Test_IntegrationRunAll(void);

/* ============================================================================
   INTEGRATION TEST SUITES
   ========================================================================== */

/**
 * @brief SUITE 1: State Machine & Synchronization
 * Tests AppState_Init, Snapshot, mutex protection
 * @return 1 if passed, 0 if failed
 */
uint32_t test_suite_state_machine(void);

/**
 * @brief SUITE 2: Control Logic & Torque Calculation
 * Tests Control_Init, Control_Step10ms, torque computation
 * at various throttle levels
 * @return 1 if passed, 0 if failed
 */
uint32_t test_suite_control_logic(void);

/**
 * @brief SUITE 3: CAN Communication (RX/TX)
 * Tests CAN_Pack16, CAN_Unpack16, queue operations
 * with multiple messages
 * @return 1 if passed, 0 if failed
 */
uint32_t test_suite_can_communication(void);

/**
 * @brief SUITE 4: Telemetry & Logging
 * Tests Telemetry_Build32, Telemetry_Send32
 * @return 1 if passed, 0 if failed
 */
uint32_t test_suite_telemetry(void);

/**
 * @brief SUITE 5: Safety Logic (EV2.3, Plausibility)
 * Tests EV2.3 flag, throttle/brake inhibition logic
 * @return 1 if passed, 0 if failed
 */
uint32_t test_suite_safety_logic(void);

/**
 * @brief SUITE 6: Multi-Task Concurrency & Timing
 * Tests that ControlTask, CanRxTask, CanTxTask run
 * without race conditions or deadlocks
 * @return 1 if passed, 0 if failed
 */
uint32_t test_suite_concurrency(void);

/**
 * @brief SUITE 7: Full System Integration
 * Tests complete workflow: sensor input → control → CAN output
 * @return 1 if passed, 0 if failed
 */
uint32_t test_suite_full_integration(void);

/**
 * @brief SUITE 8: Stress & Edge Cases
 * Tests boundary conditions, rapid state changes, queue overflows
 * @return 1 if passed, 0 if failed
 */
uint32_t test_suite_stress_testing(void);

#ifdef __cplusplus
}
#endif

#endif /* TEST_INTEGRATION_H */
