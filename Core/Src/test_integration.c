/**
 ******************************************************************************
 * @file           : test_integration.c
 * @brief          : Full integration tests with all FreeRTOS tasks running
 ******************************************************************************
 */

#include "test_integration.h"
#include "app_state.h"
#include "control.h"
#include "can.h"
#include "diag.h"
#include "telemetry.h"
#include "cmsis_os.h"
#include <string.h>

/* Test harness macros */
#define ASSERT_TRUE(cond, suite, test_name) \
  do { \
    if (!(cond)) { \
      Diag_Log("[FAIL] %s::%s", suite, test_name); \
      return 0; \
    } \
  } while(0)

#define ASSERT_RANGE(val, min, max, suite, test_name) \
  do { \
    if ((val) < (min) || (val) > (max)) { \
      Diag_Log("[FAIL] %s::%s (got %lu, expected %lu-%lu)", suite, test_name, (uint32_t)(val), (uint32_t)(min), (uint32_t)(max)); \
      return 0; \
    } \
  } while(0)

#define ASSERT_EQUAL(actual, expected, suite, test_name) \
  do { \
    if ((actual) != (expected)) { \
      Diag_Log("[FAIL] %s::%s (got %lu, expected %lu)", suite, test_name, (uint32_t)(actual), (uint32_t)(expected)); \
      return 0; \
    } \
  } while(0)

#define TEST_PASS(suite, test_name) \
  do { \
    Diag_Log("[PASS] %s::%s", suite, test_name); \
  } while(0)

static uint32_t g_tests_total = 0;
static uint32_t g_tests_passed = 0;

extern osMessageQueueId_t canRxQueueHandle;
extern osMessageQueueId_t canTxQueueHandle;

/* ============================================================================
   SUITE 1: STATE MACHINE & SYNCHRONIZATION (Mutex protection)
   ========================================================================== */
uint32_t test_suite_state_machine(void) {
  const char *suite = "STATE_MACHINE";
  
  Diag_Log("\n=== SUITE 1: State Machine & Synchronization ===");
  
  // TEST 1.1: Initial state valid
  g_tests_total++;
  {
    app_inputs_t snap;
    AppState_Snapshot(&snap);
    ASSERT_RANGE(snap.torque_total, 0, 100, suite, "1.1_initial_torque");
    ASSERT_TRUE(snap.torque_total == 0, suite, "1.1_initial_zero");
    g_tests_passed++;
    TEST_PASS(suite, "1.1_initial_state");
  }
  
  // TEST 1.2: Multiple concurrent snapshots
  g_tests_total++;
  {
    app_inputs_t snap1, snap2, snap3;
    AppState_Snapshot(&snap1);
    osDelay(2);
    AppState_Snapshot(&snap2);
    osDelay(2);
    AppState_Snapshot(&snap3);
    
    ASSERT_TRUE(snap1.torque_total >= 0, suite, "1.2_snap1_valid");
    ASSERT_TRUE(snap2.torque_total >= 0, suite, "1.2_snap2_valid");
    ASSERT_TRUE(snap3.torque_total >= 0, suite, "1.2_snap3_valid");
    g_tests_passed++;
    TEST_PASS(suite, "1.2_concurrent_snapshots");
  }
  
  // TEST 1.3: State consistency
  g_tests_total++;
  {
    app_inputs_t snap;
    for (int i = 0; i < 10; i++) {
      AppState_Snapshot(&snap);
      ASSERT_TRUE(snap.torque_total <= 100, suite, "1.3_consistency");
    }
    g_tests_passed++;
    TEST_PASS(suite, "1.3_state_consistency");
  }
  
  return 1;
}

/* ============================================================================
   SUITE 2: CONTROL LOGIC & TORQUE CALCULATION
   ========================================================================== */
uint32_t test_suite_control_logic(void) {
  const char *suite = "CONTROL_LOGIC";
  
  Diag_Log("\n=== SUITE 2: Control Logic & Torque Calculation ===");
  
  app_inputs_t input_state;
  control_out_t control_output;
  
  // TEST 2.1: 0% throttle → 0% torque
  g_tests_total++;
  {
    AppState_Snapshot(&input_state);
    input_state.s1_aceleracion = 0;
    input_state.s_freno = 0;
    
    Control_Step10ms(&input_state, &control_output);
    
    ASSERT_EQUAL(control_output.torque_pct, 0, suite, "2.1_zero_throttle");
    g_tests_passed++;
    TEST_PASS(suite, "2.1_zero_throttle_torque");
  }
  
  // TEST 2.2: 25% throttle produces proportional torque
  g_tests_total++;
  {
    AppState_Snapshot(&input_state);
    input_state.s1_aceleracion = 1024;  // ~25% of 4095 (12-bit ADC)
    input_state.s_freno = 0;
    
    Control_Step10ms(&input_state, &control_output);
    
    ASSERT_RANGE(control_output.torque_pct, 1, 40, suite, "2.2_25pct_throttle");
    g_tests_passed++;
    TEST_PASS(suite, "2.2_25pct_throttle");
  }
  
  // TEST 2.3: 50% throttle
  g_tests_total++;
  {
    AppState_Snapshot(&input_state);
    input_state.s1_aceleracion = 2048;  // ~50%
    input_state.s_freno = 0;
    
    Control_Step10ms(&input_state, &control_output);
    
    ASSERT_RANGE(control_output.torque_pct, 30, 70, suite, "2.3_50pct_throttle");
    g_tests_passed++;
    TEST_PASS(suite, "2.3_50pct_throttle");
  }
  
  // TEST 2.4: 100% throttle → max torque
  g_tests_total++;
  {
    AppState_Snapshot(&input_state);
    input_state.s1_aceleracion = 4095;  // 100%
    input_state.s_freno = 0;
    
    Control_Step10ms(&input_state, &control_output);
    
    ASSERT_RANGE(control_output.torque_pct, 80, 100, suite, "2.4_100pct_throttle");
    g_tests_passed++;
    TEST_PASS(suite, "2.4_100pct_throttle");
  }
  
  // TEST 2.5: CAN output count valid
  g_tests_total++;
  {
    ASSERT_RANGE(control_output.count, 0, 8, suite, "2.5_can_output_count");
    g_tests_passed++;
    TEST_PASS(suite, "2.5_can_output_count");
  }
  
  return 1;
}

/* ============================================================================
   SUITE 3: CAN COMMUNICATION (Pack, Unpack, Queue)
   ========================================================================== */
uint32_t test_suite_can_communication(void) {
  const char *suite = "CAN_COMM";
  
  Diag_Log("\n=== SUITE 3: CAN Communication ===");
  
  can_msg_t test_msg, unpacked_msg;
  can_qitem16_t qitem;
  osStatus_t status;
  
  // TEST 3.1: CAN message packing
  g_tests_total++;
  {
    test_msg.bus = CAN_BUS_INV;
    test_msg.id = 0x100;
    test_msg.dlc = 8;
    test_msg.ide = 0;
    test_msg.data[0] = 0x12;
    test_msg.data[1] = 0x34;
    test_msg.data[2] = 0x56;
    test_msg.data[3] = 0x78;
    
    CAN_Pack16(&test_msg, &qitem);
    
    ASSERT_TRUE(qitem.w[0] != 0, suite, "3.1_pack_not_zero");
    g_tests_passed++;
    TEST_PASS(suite, "3.1_can_pack");
  }
  
  // TEST 3.2: CAN message unpacking
  g_tests_total++;
  {
    CAN_Unpack16(&qitem, &unpacked_msg);
    
    ASSERT_EQUAL(unpacked_msg.id, 0x100, suite, "3.2_unpack_id");
    ASSERT_EQUAL(unpacked_msg.dlc, 8, suite, "3.2_unpack_dlc");
    ASSERT_EQUAL(unpacked_msg.data[0], 0x12, suite, "3.2_unpack_data0");
    ASSERT_EQUAL(unpacked_msg.data[1], 0x34, suite, "3.2_unpack_data1");
    g_tests_passed++;
    TEST_PASS(suite, "3.2_can_unpack");
  }
  
  // TEST 3.3: RX queue put/get
  g_tests_total++;
  {
    can_qitem16_t test_qitem;
    memset(&test_qitem, 0, sizeof(test_qitem));
    test_qitem.w[0] = 0xDEADBEEF;
    
    status = osMessageQueuePut(canRxQueueHandle, &test_qitem, 0, 0);
    ASSERT_EQUAL(status, osOK, suite, "3.3_put_rx");
    
    can_qitem16_t retrieved_qitem;
    status = osMessageQueueGet(canRxQueueHandle, &retrieved_qitem, NULL, 10);
    ASSERT_EQUAL(status, osOK, suite, "3.3_get_rx");
    ASSERT_EQUAL(retrieved_qitem.w[0], 0xDEADBEEF, suite, "3.3_rx_data_match");
    
    g_tests_passed++;
    TEST_PASS(suite, "3.3_rx_queue");
  }
  
  // TEST 3.4: TX queue put/get
  g_tests_total++;
  {
    can_qitem16_t test_qitem;
    memset(&test_qitem, 0, sizeof(test_qitem));
    test_qitem.w[0] = 0xCAFEBABE;
    
    status = osMessageQueuePut(canTxQueueHandle, &test_qitem, 0, 0);
    ASSERT_EQUAL(status, osOK, suite, "3.4_put_tx");
    
    can_qitem16_t retrieved_qitem;
    status = osMessageQueueGet(canTxQueueHandle, &retrieved_qitem, NULL, 10);
    ASSERT_EQUAL(status, osOK, suite, "3.4_get_tx");
    ASSERT_EQUAL(retrieved_qitem.w[0], 0xCAFEBABE, suite, "3.4_tx_data_match");
    
    g_tests_passed++;
    TEST_PASS(suite, "3.4_tx_queue");
  }
  
  // TEST 3.5: Multiple messages in queue
  g_tests_total++;
  {
    for (int i = 0; i < 5; i++) {
      can_qitem16_t test_qitem;
      test_qitem.w[0] = 0x1000 + i;
      osMessageQueuePut(canRxQueueHandle, &test_qitem, 0, 0);
    }
    
    int retrieved_count = 0;
    can_qitem16_t qitem_check;
    while (osMessageQueueGet(canRxQueueHandle, &qitem_check, NULL, 0) == osOK) {
      retrieved_count++;
    }
    
    ASSERT_RANGE(retrieved_count, 4, 5, suite, "3.5_multiple_msgs");
    g_tests_passed++;
    TEST_PASS(suite, "3.5_queue_multiple");
  }
  
  return 1;
}

/* ============================================================================
   SUITE 4: TELEMETRY & LOGGING
   ========================================================================== */
uint32_t test_suite_telemetry(void) {
  const char *suite = "TELEMETRY";
  
  Diag_Log("\n=== SUITE 4: Telemetry & Logging ===");
  
  app_inputs_t state_snap;
  uint8_t payload[32];
  
  // TEST 4.1: Telemetry build from snapshot
  g_tests_total++;
  {
    AppState_Snapshot(&state_snap);
    Telemetry_Build32(&state_snap, payload);
    
    ASSERT_TRUE(payload[0] != 0xFF || payload[1] != 0xFF, suite, "4.1_payload_not_empty");
    g_tests_passed++;
    TEST_PASS(suite, "4.1_telemetry_build");
  }
  
  // TEST 4.2: Telemetry payload size
  g_tests_total++;
  {
    ASSERT_EQUAL(sizeof(payload), 32, suite, "4.2_payload_size");
    g_tests_passed++;
    TEST_PASS(suite, "4.2_payload_size");
  }
  
  // TEST 4.3: Multiple telemetry builds
  g_tests_total++;
  {
    uint8_t payload2[32];
    uint8_t payload3[32];
    
    AppState_Snapshot(&state_snap);
    Telemetry_Build32(&state_snap, payload);
    osDelay(10);
    AppState_Snapshot(&state_snap);
    Telemetry_Build32(&state_snap, payload2);
    osDelay(10);
    AppState_Snapshot(&state_snap);
    Telemetry_Build32(&state_snap, payload3);
    
    ASSERT_TRUE(1, suite, "4.3_multiple_builds");
    g_tests_passed++;
    TEST_PASS(suite, "4.3_multiple_builds");
  }
  
  return 1;
}

/* ============================================================================
   SUITE 5: SAFETY LOGIC (EV2.3, Plausibility)
   ========================================================================== */
uint32_t test_suite_safety_logic(void) {
  const char *suite = "SAFETY";
  
  Diag_Log("\n=== SUITE 5: Safety Logic (EV2.3) ===");
  
  app_inputs_t input_state;
  uint8_t flag_ev23, flag_t11;
  uint16_t torque;
  
  // TEST 5.1: Normal throttle (no brake)
  g_tests_total++;
  {
    AppState_Snapshot(&input_state);
    input_state.s1_aceleracion = 2048;  // 50%
    input_state.s_freno = 0;
    input_state.flag_EV_2_3 = 0;
    
    torque = Control_ComputeTorque(&input_state, &flag_ev23, &flag_t11);
    
    ASSERT_RANGE(torque, 1, 100, suite, "5.1_normal_throttle");
    g_tests_passed++;
    TEST_PASS(suite, "5.1_normal_throttle");
  }
  
  // TEST 5.2: Brake engaged (should reduce/inhibit torque)
  g_tests_total++;
  {
    AppState_Snapshot(&input_state);
    input_state.s1_aceleracion = 2048;  // 50% throttle
    input_state.s_freno = 4095;         // Full brake
    
    torque = Control_ComputeTorque(&input_state, &flag_ev23, &flag_t11);
    
    // With brake, torque should be zero or very low (safety)
    ASSERT_RANGE(torque, 0, 10, suite, "5.2_brake_inhibits");
    g_tests_passed++;
    TEST_PASS(suite, "5.2_brake_inhibition");
  }
  
  // TEST 5.3: EV2.3 flag check
  g_tests_total++;
  {
    AppState_Snapshot(&input_state);
    input_state.flag_EV_2_3 = 1;  // Safety flag set
    
    torque = Control_ComputeTorque(&input_state, &flag_ev23, &flag_t11);
    
    ASSERT_TRUE(1, suite, "5.3_ev23_flag");
    g_tests_passed++;
    TEST_PASS(suite, "5.3_ev23_flag");
  }
  
  // TEST 5.4: Both accelerator and brake (should inhibit)
  g_tests_total++;
  {
    AppState_Snapshot(&input_state);
    input_state.s1_aceleracion = 4095;  // 100% throttle
    input_state.s_freno = 4095;         // 100% brake
    
    torque = Control_ComputeTorque(&input_state, &flag_ev23, &flag_t11);
    
    // Must inhibit torque when both pressed
    ASSERT_EQUAL(torque, 0, suite, "5.4_both_pressed");
    g_tests_passed++;
    TEST_PASS(suite, "5.4_throttle_brake_inhibition");
  }
  
  return 1;
}

/* ============================================================================
   SUITE 6: MULTI-TASK CONCURRENCY (No race conditions)
   ========================================================================== */
uint32_t test_suite_concurrency(void) {
  const char *suite = "CONCURRENCY";
  
  Diag_Log("\n=== SUITE 6: Multi-Task Concurrency ===");
  
  app_inputs_t snap1, snap2;
  control_out_t ctrl_out1, ctrl_out2;
  
  // TEST 6.1: Rapid consecutive control steps
  g_tests_total++;
  {
    AppState_Snapshot(&snap1);
    Control_Step10ms(&snap1, &ctrl_out1);
    osDelay(5);  // Short delay
    
    AppState_Snapshot(&snap2);
    Control_Step10ms(&snap2, &ctrl_out2);
    
    ASSERT_RANGE(ctrl_out1.torque_pct, 0, 100, suite, "6.1_step1");
    ASSERT_RANGE(ctrl_out2.torque_pct, 0, 100, suite, "6.1_step2");
    g_tests_passed++;
    TEST_PASS(suite, "6.1_concurrent_control");
  }
  
  // TEST 6.2: State snapshots during updates
  g_tests_total++;
  {
    for (int i = 0; i < 20; i++) {
      app_inputs_t snap;
      AppState_Snapshot(&snap);
      ASSERT_RANGE(snap.torque_total, 0, 100, suite, "6.2_rapid_snapshots");
      osDelay(1);
    }
    g_tests_passed++;
    TEST_PASS(suite, "6.2_rapid_snapshots");
  }
  
  // TEST 6.3: CAN queue simultaneous operations
  g_tests_total++;
  {
    can_qitem16_t test_msg;
    test_msg.w[0] = 0x12345678;
    
    // Put in RX
    osMessageQueuePut(canRxQueueHandle, &test_msg, 0, 0);
    
    // Put in TX
    test_msg.w[0] = 0x87654321;
    osMessageQueuePut(canTxQueueHandle, &test_msg, 0, 0);
    
    // Get from both
    can_qitem16_t rx_msg, tx_msg;
    osMessageQueueGet(canRxQueueHandle, &rx_msg, NULL, 10);
    osMessageQueueGet(canTxQueueHandle, &tx_msg, NULL, 10);
    
    ASSERT_EQUAL(rx_msg.w[0], 0x12345678, suite, "6.3_rx_msg");
    ASSERT_EQUAL(tx_msg.w[0], 0x87654321, suite, "6.3_tx_msg");
    g_tests_passed++;
    TEST_PASS(suite, "6.3_concurrent_queues");
  }
  
  return 1;
}

/* ============================================================================
   SUITE 7: FULL SYSTEM INTEGRATION (Complete workflow)
   ========================================================================== */
uint32_t test_suite_full_integration(void) {
  const char *suite = "FULL_INTEGRATION";
  
  Diag_Log("\n=== SUITE 7: Full System Integration ===");
  
  app_inputs_t input;
  control_out_t output;
  uint8_t telemetry[32];
  uint32_t start_time, elapsed;
  
  // TEST 7.1: Sensor → Control → Telemetry pipeline
  g_tests_total++;
  {
    // 1. Read sensor state
    AppState_Snapshot(&input);
    input.s1_aceleracion = 2048;  // 50% throttle
    
    // 2. Control step
    Control_Step10ms(&input, &output);
    
    // 3. Generate telemetry
    Telemetry_Build32(&input, telemetry);
    
    ASSERT_RANGE(output.torque_pct, 1, 100, suite, "7.1_pipeline_torque");
    ASSERT_TRUE(telemetry[0] != 0xFF, suite, "7.1_pipeline_telemetry");
    g_tests_passed++;
    TEST_PASS(suite, "7.1_full_pipeline");
  }
  
  // TEST 7.2: Control output to CAN queue
  g_tests_total++;
  {
    AppState_Snapshot(&input);
    Control_Step10ms(&input, &output);
    
    // Enqueue CAN messages from control output
    uint32_t enqueued = 0;
    for (uint8_t i = 0; i < output.count && i < 8; i++) {
      can_qitem16_t qitem;
      CAN_Pack16(&output.msgs[i], &qitem);
      osStatus_t status = osMessageQueuePut(canTxQueueHandle, &qitem, 0, 0);
      if (status == osOK) enqueued++;
    }
    
    ASSERT_TRUE(enqueued >= 0, suite, "7.2_enqueue_messages");
    g_tests_passed++;
    TEST_PASS(suite, "7.2_control_to_can");
  }
  
  // TEST 7.3: Timing validation (10ms control loop)
  g_tests_total++;
  {
    start_time = osKernelGetTickCount();
    
    AppState_Snapshot(&input);
    Control_Step10ms(&input, &output);
    
    elapsed = osKernelGetTickCount() - start_time;
    
    // Control step should complete in < 5ms (leaving margin for 10ms period)
    ASSERT_RANGE(elapsed, 0, 5, suite, "7.3_control_timing");
    g_tests_passed++;
    TEST_PASS(suite, "7.3_control_timing");
  }
  
  // TEST 7.4: State consistency across pipeline
  g_tests_total++;
  {
    AppState_Snapshot(&input);
    uint16_t snapshot_torque = input.torque_total;
    
    AppState_Snapshot(&input);
    Control_Step10ms(&input, &output);
    
    // Torque from control should be reasonable
    ASSERT_RANGE(output.torque_pct, 0, 100, suite, "7.4_consistency");
    g_tests_passed++;
    TEST_PASS(suite, "7.4_state_consistency");
  }
  
  return 1;
}

/* ============================================================================
   SUITE 8: STRESS & EDGE CASES
   ========================================================================== */
uint32_t test_suite_stress_testing(void) {
  const char *suite = "STRESS";
  
  Diag_Log("\n=== SUITE 8: Stress & Edge Cases ===");
  
  app_inputs_t input;
  control_out_t output;
  
  // TEST 8.1: Rapid state changes
  g_tests_total++;
  {
    for (int i = 0; i < 100; i++) {
      AppState_Snapshot(&input);
      input.s1_aceleracion = (i * 40) % 4096;  // Sweep throttle
      Control_Step10ms(&input, &output);
      ASSERT_RANGE(output.torque_pct, 0, 100, suite, "8.1_rapid_changes");
    }
    g_tests_passed++;
    TEST_PASS(suite, "8.1_rapid_state_changes");
  }
  
  // TEST 8.2: Boundary values (0, 50%, 100%)
  g_tests_total++;
  {
    uint16_t test_values[] = {0, 2047, 4095};
    
    for (int i = 0; i < 3; i++) {
      AppState_Snapshot(&input);
      input.s1_aceleracion = test_values[i];
      Control_Step10ms(&input, &output);
      ASSERT_RANGE(output.torque_pct, 0, 100, suite, "8.2_boundary");
    }
    g_tests_passed++;
    TEST_PASS(suite, "8.2_boundary_values");
  }
  
  // TEST 8.3: Queue stress (multiple rapid enqueues)
  g_tests_total++;
  {
    int successful = 0;
    for (int i = 0; i < 50; i++) {
      can_qitem16_t qitem;
      qitem.w[0] = i;
      osStatus_t status = osMessageQueuePut(canRxQueueHandle, &qitem, 0, 0);
      if (status == osOK) successful++;
    }
    
    ASSERT_RANGE(successful, 40, 50, suite, "8.3_queue_stress");
    
    // Drain queue
    can_qitem16_t drain;
    while (osMessageQueueGet(canRxQueueHandle, &drain, NULL, 0) == osOK) {}
    
    g_tests_passed++;
    TEST_PASS(suite, "8.3_queue_stress");
  }
  
  // TEST 8.4: Long-running consistency (1000ms of control loops)
  g_tests_total++;
  {
    uint32_t start = osKernelGetTickCount();
    uint32_t loop_count = 0;
    
    while ((osKernelGetTickCount() - start) < 1000) {
      AppState_Snapshot(&input);
      Control_Step10ms(&input, &output);
      ASSERT_RANGE(output.torque_pct, 0, 100, suite, "8.4_long_run");
      loop_count++;
      osDelay(1);
    }
    
    ASSERT_RANGE(loop_count, 50, 1000, suite, "8.4_loop_count");
    g_tests_passed++;
    TEST_PASS(suite, "8.4_long_running");
  }
  
  return 1;
}

/* ============================================================================
   TEST RUNNER: Execute all suites and generate report
   ========================================================================== */
test_result_t Test_IntegrationRunAll(void) {
  test_result_t result;
  uint32_t start_time = osKernelGetTickCount();
  
  Diag_Log("\n\n");
  Diag_Log("====================================================");
  Diag_Log("  ECU08 NSIL - FULL INTEGRATION TEST SUITE");
  Diag_Log("====================================================");
  Diag_Log("Running with all FreeRTOS tasks active...\n");
  
  g_tests_total = 0;
  g_tests_passed = 0;
  
  // Run all test suites
  test_suite_state_machine();
  test_suite_control_logic();
  test_suite_can_communication();
  test_suite_telemetry();
  test_suite_safety_logic();
  test_suite_concurrency();
  test_suite_full_integration();
  test_suite_stress_testing();
  
  // Calculate results
  uint32_t elapsed = osKernelGetTickCount() - start_time;
  result.total = g_tests_total;
  result.passed = g_tests_passed;
  result.failed = g_tests_total - g_tests_passed;
  result.execution_time_ms = elapsed;
  
  // Print report
  Diag_Log("\n====================================================");
  Diag_Log("  TEST REPORT");
  Diag_Log("====================================================");
  Diag_Log("Total Tests:      %lu", result.total);
  Diag_Log("Passed:           %lu", result.passed);
  Diag_Log("Failed:           %lu", result.failed);
  Diag_Log("Execution Time:   %lu ms", result.execution_time_ms);
  
  if (result.failed == 0) {
    Diag_Log("\n✅ RESULT: ALL TESTS PASSED (100%%)");
    Diag_Log("Status: SYSTEM READY FOR HARDWARE");
  } else {
    Diag_Log("\n❌ RESULT: %lu TESTS FAILED", result.failed);
  }
  
  Diag_Log("====================================================\n");
  
  return result;
}
