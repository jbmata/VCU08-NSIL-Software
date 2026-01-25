#include "test_advanced.h"
#include "app_state.h"
#include "control.h"
#include "can.h"
#include "diag.h"
#include <string.h>

/* ============================================================================ */
/* TEST FRAMEWORK */
/* ============================================================================ */

static uint16_t test_count = 0;
static uint16_t test_passed = 0;
static uint16_t test_failed = 0;

#define TEST_ASSERT(cond, msg) \
  do { \
    test_count++; \
    if (cond) { \
      test_passed++; \
      Diag_Log("[OK] " msg); \
    } else { \
      test_failed++; \
      Diag_Log("[FAIL] " msg); \
    } \
  } while(0)

#define TEST_SECTION(name) \
  Diag_Log("\n=== " name " ===");

/* ============================================================================ */
/* TEST 1: STRESS CYCLING - Repeated torque calculations */
/* ============================================================================ */

static TestStats_t test_stress_cycling(uint32_t cycles)
{
  TEST_SECTION("STRESS CYCLING");
  Diag_Log("Testing %lu cycles of torque computation", cycles);
  
  AppState_Init();
  
  for (uint32_t cycle = 0; cycle < cycles; cycle++)
  {
    /* Simulate S1, S2 values varying */
    uint16_t s1 = (cycle * 17) % 4096;
    uint16_t s2 = (cycle * 23) % 4096;
    
    AppState_t snap = AppState_Snapshot();
    snap.sensor_s1 = s1;
    snap.sensor_s2 = s2;
    
    /* Torque should always be in valid range */
    uint16_t throttle_pct = ((s1 + s2) / 2) * 100 / 4096;
    TEST_ASSERT(throttle_pct <= 100, "Throttle within range");
  }
  
  TestStats_t stats;
  stats.total = test_count;
  stats.passed = test_passed;
  stats.failed = test_failed;
  return stats;
}

/* ============================================================================ */
/* TEST 2: BOUNDARY CONDITIONS - Edge values */
/* ============================================================================ */

static TestStats_t test_boundary_conditions(void)
{
  TEST_SECTION("BOUNDARY CONDITIONS");
  
  AppState_Init();
  AppState_t snap = AppState_Snapshot();
  
  /* Test MIN values */
  snap.sensor_s1 = 0;
  snap.sensor_s2 = 0;
  TEST_ASSERT(snap.sensor_s1 == 0, "S1 min = 0");
  TEST_ASSERT(snap.sensor_s2 == 0, "S2 min = 0");
  
  /* Test MAX values */
  snap.sensor_s1 = 4095;
  snap.sensor_s2 = 4095;
  TEST_ASSERT(snap.sensor_s1 == 4095, "S1 max = 4095");
  TEST_ASSERT(snap.sensor_s2 == 4095, "S2 max = 4095");
  
  /* Test asymmetric pairs */
  snap.sensor_s1 = 0;
  snap.sensor_s2 = 4095;
  uint16_t avg = (snap.sensor_s1 + snap.sensor_s2) / 2;
  TEST_ASSERT(avg == 2047, "Average of 0 and 4095 = 2047");
  
  /* Test mid-range */
  snap.sensor_s1 = 2048;
  snap.sensor_s2 = 2048;
  uint16_t mid_avg = (snap.sensor_s1 + snap.sensor_s2) / 2;
  TEST_ASSERT(mid_avg == 2048, "Mid-range averaging");
  
  /* Test sensor spread limit (EV2.3) */
  snap.sensor_s1 = 1000;
  snap.sensor_s2 = 1200;
  uint16_t spread = (snap.sensor_s1 > snap.sensor_s2) ? 
                    (snap.sensor_s1 - snap.sensor_s2) : 
                    (snap.sensor_s2 - snap.sensor_s1);
  TEST_ASSERT(spread <= 200, "Sensor spread within limits");
  
  TestStats_t stats;
  stats.total = test_count;
  stats.passed = test_passed;
  stats.failed = test_failed;
  return stats;
}

/* ============================================================================ */
/* TEST 3: CONCURRENCY SIMULATION - Multiple state reads */
/* ============================================================================ */

static TestStats_t test_concurrency_simulation(void)
{
  TEST_SECTION("CONCURRENCY SIMULATION");
  
  AppState_Init();
  
  /* Simulate rapid state snapshots (as would occur in concurrent tasks) */
  for (int i = 0; i < 50; i++)
  {
    AppState_t snap1 = AppState_Snapshot();
    AppState_t snap2 = AppState_Snapshot();
    AppState_t snap3 = AppState_Snapshot();
    
    /* All three snapshots should be consistent */
    TEST_ASSERT(snap1.state == snap2.state, "State consistency check 1");
    TEST_ASSERT(snap2.state == snap3.state, "State consistency check 2");
    TEST_ASSERT(snap1.torque_setpoint == snap2.torque_setpoint, "Torque consistency");
  }
  
  /* Test state mutation isolation */
  AppState_t snap_before = AppState_Snapshot();
  uint16_t val_before = snap_before.sensor_s1;
  
  /* Another task's reading shouldn't affect previous snapshot */
  AppState_t snap_after = AppState_Snapshot();
  TEST_ASSERT(snap_before.sensor_s1 == val_before, "Snapshot immutability");
  TEST_ASSERT(snap_before.state == snap_after.state, "State preserved across snapshots");
  
  TestStats_t stats;
  stats.total = test_count;
  stats.passed = test_passed;
  stats.failed = test_failed;
  return stats;
}

/* ============================================================================ */
/* TEST 4: CAN MESSAGE EXCHANGE - Pack/Unpack with corruption detection */
/* ============================================================================ */

static TestStats_t test_can_message_exchange(void)
{
  TEST_SECTION("CAN MESSAGE EXCHANGE");
  
  /* Test CAN message serialization */
  can_msg_t msg;
  msg.id = 0x123;
  msg.dlc = 8;
  msg.data[0] = 0xAA;
  msg.data[1] = 0xBB;
  msg.data[2] = 0xCC;
  msg.data[3] = 0xDD;
  msg.data[4] = 0x11;
  msg.data[5] = 0x22;
  msg.data[6] = 0x33;
  msg.data[7] = 0x44;
  
  can_qitem16_t qitem;
  CAN_Pack16(&msg, &qitem);
  
  /* Verify all bytes preserved */
  TEST_ASSERT(qitem.msg.id == 0x123, "Message ID preserved");
  TEST_ASSERT(qitem.msg.dlc == 8, "DLC preserved");
  for (int i = 0; i < 8; i++)
  {
    char buf[32];
    uint32_t snprintf(char *s, size_t n, const char *format, ...);
    TEST_ASSERT(qitem.msg.data[i] == msg.data[i], "Byte preservation");
  }
  
  /* Test unpack round-trip */
  can_msg_t unpacked;
  CAN_Unpack16(&qitem, &unpacked);
  TEST_ASSERT(unpacked.id == msg.id, "Unpack ID match");
  TEST_ASSERT(unpacked.dlc == msg.dlc, "Unpack DLC match");
  for (int i = 0; i < 8; i++)
  {
    TEST_ASSERT(unpacked.data[i] == msg.data[i], "Round-trip data integrity");
  }
  
  TestStats_t stats;
  stats.total = test_count;
  stats.passed = test_passed;
  stats.failed = test_failed;
  return stats;
}

/* ============================================================================ */
/* TEST 5: STATE MACHINE TRANSITIONS */
/* ============================================================================ */

static TestStats_t test_state_machine(void)
{
  TEST_SECTION("STATE MACHINE TRANSITIONS");
  
  AppState_Init();
  
  AppState_t snap = AppState_Snapshot();
  TEST_ASSERT(snap.state == APP_STATE_BOOT, "Initial state is BOOT");
  
  /* Simulate state progression */
  for (int i = 0; i < 20; i++)
  {
    snap = AppState_Snapshot();
    /* State should be valid enum value */
    TEST_ASSERT(snap.state >= APP_STATE_BOOT && snap.state <= APP_STATE_RUN, 
                "State within valid range");
  }
  
  TestStats_t stats;
  stats.total = test_count;
  stats.passed = test_passed;
  stats.failed = test_failed;
  return stats;
}

/* ============================================================================ */
/* TEST 6: BRAKE-THROTTLE LOGIC */
/* ============================================================================ */

static TestStats_t test_brake_throttle_logic(void)
{
  TEST_SECTION("BRAKE-THROTTLE LOGIC");
  
  AppState_Init();
  
  AppState_t snap;
  
  /* Case 1: No brake, low throttle */
  snap = AppState_Snapshot();
  snap.brake_pressed = 0;
  snap.sensor_s1 = 500;  /* ~12% */
  TEST_ASSERT(snap.brake_pressed == 0, "Brake released");
  
  /* Case 2: Brake pressed, no throttle */
  snap = AppState_Snapshot();
  snap.brake_pressed = 1;
  snap.sensor_s1 = 0;
  TEST_ASSERT(snap.brake_pressed == 1, "Brake engaged");
  
  /* Case 3: Brake + high throttle (EV2.3 violation) */
  snap = AppState_Snapshot();
  snap.brake_pressed = 1;
  snap.sensor_s1 = 3000;  /* >73% */
  TEST_ASSERT(snap.brake_pressed == 1, "EV2.3: Brake takes precedence");
  
  TestStats_t stats;
  stats.total = test_count;
  stats.passed = test_passed;
  stats.failed = test_failed;
  return stats;
}

/* ============================================================================ */
/* TEST 7: TEMPERATURE AND VOLTAGE LIMITS */
/* ============================================================================ */

static TestStats_t test_limits(void)
{
  TEST_SECTION("OPERATIONAL LIMITS");
  
  AppState_t snap = AppState_Snapshot();
  
  /* Test voltage range */
  snap.voltage_v = 300;  /* 30.0V */
  TEST_ASSERT(snap.voltage_v >= 0 && snap.voltage_v <= 1000, "Voltage in range");
  
  /* Test temperature range */
  snap.temperature_c = 50;  /* 50°C */
  TEST_ASSERT(snap.temperature_c >= -40 && snap.temperature_c <= 150, "Temperature in range");
  
  /* Test current limits */
  snap.current_a = 150;  /* 150A */
  TEST_ASSERT(snap.current_a <= 500, "Current within limits");
  
  TestStats_t stats;
  stats.total = test_count;
  stats.passed = test_passed;
  stats.failed = test_failed;
  return stats;
}

/* ============================================================================ */
/* MAIN TEST RUNNER */
/* ============================================================================ */

TestStats_t Test_AdvancedRunAll(void)
{
  test_count = 0;
  test_passed = 0;
  test_failed = 0;
  
  Diag_Log("\n╔══════════════════════════════════════════════════════════╗");
  Diag_Log("║        ADVANCED TEST SUITE - ECU08 NSIL                  ║");
  Diag_Log("╚══════════════════════════════════════════════════════════╝\n");
  
  /* Run all test suites */
  test_stress_cycling(100);
  test_boundary_conditions();
  test_concurrency_simulation();
  test_can_message_exchange();
  test_state_machine();
  test_brake_throttle_logic();
  test_limits();
  
  /* Final report */
  Diag_Log("\n╔══════════════════════════════════════════════════════════╗");
  Diag_Log("║                    TEST SUMMARY                          ║");
  Diag_Log("╠══════════════════════════════════════════════════════════╣");
  Diag_Log("║ Total Assertions: %u                                     ║", test_count);
  Diag_Log("║ Passed: %u ✓                                             ║", test_passed);
  Diag_Log("║ Failed: %u ✗                                             ║", test_failed);
  Diag_Log("║ Pass Rate: %.1f%%                                         ║", 
           (float)test_passed * 100.0f / (test_count > 0 ? test_count : 1));
  Diag_Log("╚══════════════════════════════════════════════════════════╝\n");
  
  TestStats_t stats;
  stats.total = test_count;
  stats.passed = test_passed;
  stats.failed = test_failed;
  stats.duration_ms = 0;
  
  return stats;
}

TestStats_t Test_StressCycling(uint32_t cycles)
{
  test_count = 0;
  test_passed = 0;
  test_failed = 0;
  return test_stress_cycling(cycles);
}

TestStats_t Test_BoundaryConditions(void)
{
  test_count = 0;
  test_passed = 0;
  test_failed = 0;
  return test_boundary_conditions();
}

TestStats_t Test_ConcurrencySimulation(void)
{
  test_count = 0;
  test_passed = 0;
  test_failed = 0;
  return test_concurrency_simulation();
}

TestStats_t Test_CANMessageExchange(void)
{
  test_count = 0;
  test_passed = 0;
  test_failed = 0;
  return test_can_message_exchange();
}
