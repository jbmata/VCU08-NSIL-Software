#include "test_startup_sequence.h"
#include "app_state.h"
#include "control.h"
#include "diag.h"
#include <string.h>

/* ============================================================================ */
/* TEST FRAMEWORK FOR STARTUP SEQUENCE */
/* ============================================================================ */

static uint16_t test_count = 0;
static uint16_t test_passed = 0;
static uint16_t test_failed = 0;

#define TEST_ASSERT(cond, msg) \
  do { \
    test_count++; \
    if (cond) { \
      test_passed++; \
      Diag_Log("[✓] " msg); \
    } else { \
      test_failed++; \
      Diag_Log("[✗] " msg); \
    } \
  } while(0)

#define TEST_SECTION(name) \
  Diag_Log("\n┌─ " name " ─────────────────────────────────────┐");

#define TEST_SUBSECTION(name) \
  Diag_Log("├─ " name);

/* ============================================================================ */
/* STARTUP SEQUENCE STATE MACHINE (Simulation) */
/* ============================================================================ */

typedef enum {
  SIM_CTRL_ST_BOOT = 0,
  SIM_CTRL_ST_WAIT_PRECHARGE_ACK,
  SIM_CTRL_ST_WAIT_START_BRAKE,
  SIM_CTRL_ST_R2D_DELAY,
  SIM_CTRL_ST_READY,
  SIM_CTRL_ST_RUN
} sim_ctrl_state_t;

typedef struct {
  sim_ctrl_state_t state;
  uint32_t ticks;
  uint16_t torque_output;
  uint8_t can_frames_sent;
} startup_sim_t;

static startup_sim_t sim;

/* Initialize startup simulator */
static void startup_sim_init(void)
{
  sim.state = SIM_CTRL_ST_BOOT;
  sim.ticks = 0;
  sim.torque_output = 0;
  sim.can_frames_sent = 0;
}

/* Simulate one step of startup sequence (corresponds to Control_Step10ms) */
static void startup_sim_step(const app_inputs_t *in)
{
  if (!in) return;

  sim.ticks += 10;  /* 10ms per step */

  switch (sim.state)
  {
    case SIM_CTRL_ST_BOOT:
      /* Initial state: check if precharge is OK */
      if (in->ok_precarga)
      {
        sim.state = SIM_CTRL_ST_WAIT_START_BRAKE;
      }
      else
      {
        sim.state = SIM_CTRL_ST_WAIT_PRECHARGE_ACK;
      }
      break;

    case SIM_CTRL_ST_WAIT_PRECHARGE_ACK:
      /* Waiting for precharge handshake */
      if (in->ok_precarga)
      {
        sim.state = SIM_CTRL_ST_WAIT_START_BRAKE;
      }
      break;

    case SIM_CTRL_ST_WAIT_START_BRAKE:
      /* Waiting for start button + brake pressed */
      if (in->boton_arranque && in->s_freno > 3000)
      {
        sim.state = SIM_CTRL_ST_R2D_DELAY;
        sim.ticks = 0;  /* Reset timer for R2D delay */
      }
      break;

    case SIM_CTRL_ST_R2D_DELAY:
      /* R2D (Ready-To-Drive) 2 second delay */
      if (sim.ticks >= 2000)
      {
        sim.state = SIM_CTRL_ST_READY;
      }
      break;

    case SIM_CTRL_ST_READY:
      /* Transition to ready state */
      sim.state = SIM_CTRL_ST_RUN;
      break;

    case SIM_CTRL_ST_RUN:
    default:
      /* Running state: compute and send torque */
      float s1_pct = ((float)in->s1_aceleracion - 2050.0f) / (29.5f - 20.5f);
      float s2_pct = ((float)in->s2_aceleracion - 1915.0f) / (25.70f - 19.15f);
      if (s1_pct < 0) s1_pct = 0;
      if (s1_pct > 100) s1_pct = 100;
      if (s2_pct < 0) s2_pct = 0;
      if (s2_pct > 100) s2_pct = 100;

      sim.torque_output = (uint16_t)((s1_pct + s2_pct) * 0.5f);
      if (sim.torque_output > 100) sim.torque_output = 100;
      sim.can_frames_sent++;
      break;
  }
}

/* ============================================================================ */
/* TEST 1: BASIC BOOT SEQUENCE */
/* ============================================================================ */

static void test_basic_boot_sequence(void)
{
  TEST_SECTION("TEST 1: Basic Boot Sequence");
  TEST_SUBSECTION("BOOT → PRECHARGE_ACK → START_BRAKE");

  app_inputs_t in;
  memset(&in, 0, sizeof(in));

  startup_sim_init();
  TEST_ASSERT(sim.state == SIM_CTRL_ST_BOOT, "Initial state is BOOT");

  /* Step 1: Without precharge ACK, should go to WAIT_PRECHARGE_ACK */
  in.ok_precarga = 0;
  startup_sim_step(&in);
  TEST_ASSERT(sim.state == SIM_CTRL_ST_WAIT_PRECHARGE_ACK, "No precharge → WAIT_PRECHARGE_ACK");

  /* Step 2: Still waiting without precharge */
  startup_sim_step(&in);
  TEST_ASSERT(sim.state == SIM_CTRL_ST_WAIT_PRECHARGE_ACK, "Stays in WAIT_PRECHARGE_ACK");

  /* Step 3: Precharge becomes available */
  in.ok_precarga = 1;
  startup_sim_step(&in);
  TEST_ASSERT(sim.state == SIM_CTRL_ST_WAIT_START_BRAKE, "Precharge OK → WAIT_START_BRAKE");
}

/* ============================================================================ */
/* TEST 2: START BUTTON + BRAKE REQUIREMENT */
/* ============================================================================ */

static void test_start_button_brake_requirement(void)
{
  TEST_SECTION("TEST 2: Start Button & Brake Requirement");
  TEST_SUBSECTION("Must have both start button AND brake pressed");

  app_inputs_t in;
  memset(&in, 0, sizeof(in));
  in.ok_precarga = 1;

  startup_sim_init();
  /* Go to WAIT_START_BRAKE state */
  startup_sim_step(&in);
  TEST_ASSERT(sim.state == SIM_CTRL_ST_WAIT_START_BRAKE, "Ready to check start conditions");

  /* Test case 1: Start button but NO brake */
  in.boton_arranque = 1;
  in.s_freno = 0;  /* Brake not pressed (ADC < 3000) */
  startup_sim_step(&in);
  TEST_ASSERT(sim.state == SIM_CTRL_ST_WAIT_START_BRAKE, "Start without brake: rejected");

  /* Test case 2: Brake but NO start button */
  in.boton_arranque = 0;
  in.s_freno = 3500;  /* Brake pressed (ADC > 3000) */
  startup_sim_step(&in);
  TEST_ASSERT(sim.state == SIM_CTRL_ST_WAIT_START_BRAKE, "Brake without start: rejected");

  /* Test case 3: BOTH start button AND brake */
  in.boton_arranque = 1;
  in.s_freno = 3500;
  startup_sim_step(&in);
  TEST_ASSERT(sim.state == SIM_CTRL_ST_R2D_DELAY, "Start + Brake → R2D_DELAY");
}

/* ============================================================================ */
/* TEST 3: R2D DELAY (2000ms) */
/* ============================================================================ */

static void test_r2d_delay(void)
{
  TEST_SECTION("TEST 3: R2D Delay (2 seconds)");
  TEST_SUBSECTION("Ready-To-Drive delay must be exactly 2000ms");

  app_inputs_t in;
  memset(&in, 0, sizeof(in));
  in.ok_precarga = 1;
  in.boton_arranque = 1;
  in.s_freno = 3500;

  startup_sim_init();
  /* Navigate to R2D_DELAY state */
  startup_sim_step(&in);  /* BOOT → WAIT_START_BRAKE */
  startup_sim_step(&in);  /* WAIT_START_BRAKE → R2D_DELAY */
  TEST_ASSERT(sim.state == SIM_CTRL_ST_R2D_DELAY, "Entered R2D_DELAY state");
  TEST_ASSERT(sim.ticks == 0, "R2D timer reset to 0ms");

  /* Simulate waiting 100ms */
  for (int i = 0; i < 10; i++) startup_sim_step(&in);
  TEST_ASSERT(sim.state == SIM_CTRL_ST_R2D_DELAY, "Still in R2D_DELAY at 100ms");
  TEST_ASSERT(sim.ticks == 100, "R2D timer at 100ms");

  /* Simulate waiting to 1500ms */
  for (int i = 0; i < 140; i++) startup_sim_step(&in);
  TEST_ASSERT(sim.state == SIM_CTRL_ST_R2D_DELAY, "Still in R2D_DELAY at 1500ms");

  /* One more step to 2010ms (should transition) */
  startup_sim_step(&in);
  TEST_ASSERT(sim.state == SIM_CTRL_ST_READY, "R2D delay complete → READY at 2000ms+");
}

/* ============================================================================ */
/* TEST 4: READY → RUN TRANSITION */
/* ============================================================================ */

static void test_ready_to_run(void)
{
  TEST_SECTION("TEST 4: READY → RUN Transition");
  TEST_SUBSECTION("Transition to RUN state for torque output");

  app_inputs_t in;
  memset(&in, 0, sizeof(in));

  startup_sim_init();
  sim.state = SIM_CTRL_ST_READY;

  startup_sim_step(&in);
  TEST_ASSERT(sim.state == SIM_CTRL_ST_RUN, "READY → RUN on next step");
}

/* ============================================================================ */
/* TEST 5: TORQUE OUTPUT IN RUN STATE */
/* ============================================================================ */

static void test_torque_in_run_state(void)
{
  TEST_SECTION("TEST 5: Torque Output in RUN State");
  TEST_SUBSECTION("Validate torque calculation and CAN output");

  app_inputs_t in;
  memset(&in, 0, sizeof(in));

  startup_sim_init();
  sim.state = SIM_CTRL_ST_RUN;

  /* Test case 1: Zero throttle (sensors at minimum) */
  in.s1_aceleracion = 2050;  /* 0% */
  in.s2_aceleracion = 1915;  /* 0% */
  startup_sim_step(&in);
  TEST_ASSERT(sim.torque_output == 0, "Zero throttle → 0% torque");
  TEST_ASSERT(sim.can_frames_sent >= 1, "CAN frame queued");

  /* Test case 2: 50% throttle */
  in.s1_aceleracion = 2500;  /* ~50% */
  in.s2_aceleracion = 2400;  /* ~50% */
  startup_sim_step(&in);
  TEST_ASSERT(sim.torque_output >= 40 && sim.torque_output <= 60, "50% throttle → ~50% torque");
  TEST_ASSERT(sim.can_frames_sent >= 2, "Multiple CAN frames queued");

  /* Test case 3: Full throttle */
  in.s1_aceleracion = 2950;  /* 100% */
  in.s2_aceleracion = 2570;  /* 100% */
  startup_sim_step(&in);
  TEST_ASSERT(sim.torque_output == 100, "Full throttle → 100% torque");
  TEST_ASSERT(sim.can_frames_sent >= 3, "Continuous CAN output");
}

/* ============================================================================ */
/* TEST 6: COMPLETE STARTUP SEQUENCE (BOOT → RUN) */
/* ============================================================================ */

static void test_complete_startup_sequence(void)
{
  TEST_SECTION("TEST 6: Complete Startup Sequence");
  TEST_SUBSECTION("Full path: BOOT → PRECHARGE_ACK → START_BRAKE → R2D → READY → RUN");

  app_inputs_t in;
  memset(&in, 0, sizeof(in));

  startup_sim_init();
  TEST_ASSERT(sim.state == SIM_CTRL_ST_BOOT, "Step 0: BOOT");

  /* Phase 1: Wait for precharge */
  in.ok_precarga = 0;
  for (int i = 0; i < 50; i++) startup_sim_step(&in);
  TEST_ASSERT(sim.state == SIM_CTRL_ST_WAIT_PRECHARGE_ACK, "Phase 1: Waiting precharge");

  /* Phase 2: Precharge arrives */
  in.ok_precarga = 1;
  startup_sim_step(&in);
  TEST_ASSERT(sim.state == SIM_CTRL_ST_WAIT_START_BRAKE, "Phase 2: Precharge OK");

  /* Phase 3: Wait for start + brake (simulating user pressing brake + start) */
  for (int i = 0; i < 50; i++)
  {
    in.s_freno = 3500;
    in.boton_arranque = 1;
    startup_sim_step(&in);
    if (sim.state == SIM_CTRL_ST_R2D_DELAY) break;
  }
  TEST_ASSERT(sim.state == SIM_CTRL_ST_R2D_DELAY, "Phase 3: Start + Brake detected");

  /* Phase 4: R2D delay (2000ms) */
  for (int i = 0; i < 220; i++) startup_sim_step(&in);
  TEST_ASSERT(sim.state >= SIM_CTRL_ST_READY, "Phase 4: R2D delay complete");

  /* Phase 5: Ready state */
  if (sim.state == SIM_CTRL_ST_READY)
  {
    startup_sim_step(&in);
  }
  TEST_ASSERT(sim.state == SIM_CTRL_ST_RUN, "Phase 5: RUN state reached");

  /* Phase 6: Verify torque output in RUN */
  in.s1_aceleracion = 2500;
  in.s2_aceleracion = 2400;
  startup_sim_step(&in);
  TEST_ASSERT(sim.torque_output > 0, "Phase 6: Torque output active");
  TEST_ASSERT(sim.can_frames_sent >= 1, "Phase 6: CAN frames transmitted");
}

/* ============================================================================ */
/* TEST 7: PRECHARGE FAILURE RECOVERY */
/* ============================================================================ */

static void test_precharge_failure_recovery(void)
{
  TEST_SECTION("TEST 7: Precharge Failure & Recovery");
  TEST_SUBSECTION("System handles precharge failure gracefully");

  app_inputs_t in;
  memset(&in, 0, sizeof(in));

  startup_sim_init();
  in.ok_precarga = 0;

  /* Wait for precharge for 100ms */
  for (int i = 0; i < 10; i++) startup_sim_step(&in);
  TEST_ASSERT(sim.state == SIM_CTRL_ST_WAIT_PRECHARGE_ACK, "Waiting for precharge");

  /* Precharge arrives after delay */
  in.ok_precarga = 1;
  startup_sim_step(&in);
  TEST_ASSERT(sim.state == SIM_CTRL_ST_WAIT_START_BRAKE, "Precharge recovered");

  /* Can now proceed to next state */
  in.boton_arranque = 1;
  in.s_freno = 3500;
  startup_sim_step(&in);
  TEST_ASSERT(sim.state == SIM_CTRL_ST_R2D_DELAY, "Normal startup proceeds after recovery");
}

/* ============================================================================ */
/* TEST 8: STATE VALIDATION (No invalid states) */
/* ============================================================================ */

static void test_state_validation(void)
{
  TEST_SECTION("TEST 8: State Machine Validation");
  TEST_SUBSECTION("All state transitions remain within valid range");

  app_inputs_t in;
  memset(&in, 0, sizeof(in));

  startup_sim_init();

  /* Simulate 1000 random-ish transitions */
  for (int i = 0; i < 1000; i++)
  {
    in.ok_precarga = (i % 50) < 40;  /* ~80% precharge available */
    in.boton_arranque = (i % 100) < 30;  /* ~30% start button */
    in.s_freno = ((i * 17) % 5000);  /* Varying brake */

    startup_sim_step(&in);

    /* Verify state is always in valid range */
    TEST_ASSERT(sim.state >= SIM_CTRL_ST_BOOT && sim.state <= SIM_CTRL_ST_RUN,
                "Valid state");
  }
}

/* ============================================================================ */
/* TEST 9: RAPID START-STOP CYCLING */
/* ============================================================================ */

static void test_rapid_cycling(void)
{
  TEST_SECTION("TEST 9: Rapid Start-Stop Cycling");
  TEST_SUBSECTION("System handles rapid on/off cycles");

  app_inputs_t in;
  memset(&in, 0, sizeof(in));
  in.ok_precarga = 1;

  for (int cycle = 0; cycle < 20; cycle++)
  {
    startup_sim_init();
    in.boton_arranque = 1;
    in.s_freno = 3500;

    /* Go through full cycle */
    for (int i = 0; i < 250; i++)
    {
      startup_sim_step(&in);
    }

    TEST_ASSERT(sim.state == SIM_CTRL_ST_RUN, "Cycle complete");
  }
}

/* ============================================================================ */
/* MAIN TEST RUNNER */
/* ============================================================================ */

TestStats_Startup_t Test_StartupSequenceRunAll(void)
{
  test_count = 0;
  test_passed = 0;
  test_failed = 0;

  Diag_Log("\n╔══════════════════════════════════════════════════════════╗");
  Diag_Log("║      STARTUP SEQUENCE TEST SUITE - ECU08 NSIL            ║");
  Diag_Log("╚══════════════════════════════════════════════════════════╝");

  /* Run all startup sequence tests */
  test_basic_boot_sequence();
  test_start_button_brake_requirement();
  test_r2d_delay();
  test_ready_to_run();
  test_torque_in_run_state();
  test_complete_startup_sequence();
  test_precharge_failure_recovery();
  test_state_validation();
  test_rapid_cycling();

  /* Final report */
  Diag_Log("\n┌──────────────────────────────────────────────────────────┐");
  Diag_Log("│                  STARTUP TEST SUMMARY                    │");
  Diag_Log("├──────────────────────────────────────────────────────────┤");
  Diag_Log("│ Total Assertions:    %u                                  │", test_count);
  Diag_Log("│ Passed:              %u ✓                                │", test_passed);
  Diag_Log("│ Failed:              %u ✗                                │", test_failed);

  float pass_rate = (test_count > 0) ? ((float)test_passed * 100.0f / test_count) : 0;
  Diag_Log("│ Pass Rate:           %.1f%%                               │", pass_rate);

  Diag_Log("└──────────────────────────────────────────────────────────┘\n");

  TestStats_Startup_t stats;
  stats.total = test_count;
  stats.passed = test_passed;
  stats.failed = test_failed;

  return stats;
}
