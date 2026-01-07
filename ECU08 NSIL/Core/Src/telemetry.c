#include "telemetry.h"
#include "cmsis_os2.h"
#include "main.h"
#include "VCU.h"
#include <string.h>
#include <stdio.h>

/* NRF24 includes tuyos */
#include "nrf24.h"

/* Globals tuyos */
volatile uint32_t tel_irq_cnt   = 0;
volatile uint32_t tel_sent_ok   = 0;
volatile uint32_t tel_sent_fail = 0;

uint32_t tel_tick = 0;

/* Datos a enviar */
extern int inv_dc_bus_voltage;
extern int e_machine_rpm;
extern uint16_t torque_total;
extern float v_celda_min;
extern uint8_t state;
extern int inv_t_motor, inv_t_igbt, inv_t_air, inv_n_actual, inv_i_actual;
extern uint16_t s1_aceleracion, s2_aceleracion;
extern int s_freno;
extern int precharge_button;
extern int start_button_act;

/* UART debug */
extern UART_HandleTypeDef huart2;

/* Config radio */
#define TEL_CHAN 76
static uint8_t rf_addr[5] = {0xE7,0xE7,0xE7,0xE7,0xE7};

typedef struct __attribute__((packed)) {
    uint16_t id;
    uint16_t seq;
    float    v1,v2,v3,v4,v5,v6,v7;
} TelFrame;

/* ===== helpers privados ===== */
static void tel_build_packet(TelFrame *p);
static uint8_t nrf24_tx32(const void *buf32);
static void nrf24_flush_tx(void);

void Telemetry_Init(void)
{
  /* Tu bring-up NRF24: muévelo aquí desde main */
  NRF24_Init();
  NRF24_TxMode(rf_addr, TEL_CHAN);

  nrf24_WriteReg(DYNPD, 0x00);
  nrf24_WriteReg(FEATURE, 0x00);
  nrf24_WriteReg(RX_PW_P0, 32);
}

void Telemetry_Task(void)
{
  for (;;)
  {
    /* 2 Hz como en tu tick 500ms */
    osDelay(500);

    TelFrame pkt;
    tel_build_packet(&pkt);

    uint8_t ok = nrf24_tx32(&pkt);
    if (ok) tel_sent_ok++; else tel_sent_fail++;
  }
}

static void tel_build_packet(TelFrame *p)
{
  static uint16_t seq = 0;
  static uint8_t which = 0;

  p->seq = seq++;

  switch (which)
  {
    default:
    case 0:
      p->id = 0x600;
      p->v1 = (float)inv_dc_bus_voltage;
      p->v2 = (float)e_machine_rpm;
      p->v3 = (float)torque_total;
      p->v4 = (float)v_celda_min;
      p->v5 = (float)state;
      p->v6 = 0.0f;
      p->v7 = 0.0f;
      break;

    case 1:
      p->id = 0x610;
      p->v1 = (float)inv_t_motor;
      p->v2 = (float)inv_t_igbt;
      p->v3 = (float)inv_t_air;
      p->v4 = (float)inv_n_actual;
      p->v5 = (float)inv_i_actual;
      p->v6 = 0.0f;
      p->v7 = 0.0f;
      break;

    case 2:
      p->id = 0x620;
      p->v1 = (float)s1_aceleracion;
      p->v2 = (float)s2_aceleracion;
      p->v3 = (float)s_freno;
      p->v4 = (float)precharge_button;
      p->v5 = (float)start_button_act;
      p->v6 = 0.0f;
      p->v7 = 0.0f;
      break;

    case 3:
      p->id = 0x630;
      p->v1 = (float)inv_dc_bus_voltage;
      p->v2 = 0.0f;
      p->v3 = 0.0f;
      p->v4 = 0.0f;
      p->v5 = 0.0f;
      p->v6 = 0.0f;
      p->v7 = 0.0f;
      break;
  }

  which = (uint8_t)((which + 1) & 0x03);
}

static void nrf24_flush_tx(void)
{
  uint8_t cmd = 0xE1; // FLUSH_TX
  HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
  HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_SET);
}

static uint8_t nrf24_tx32(const void *buf32)
{
  /* Clear IRQs */
  nrf24_WriteReg(STATUS, (1u<<6)|(1u<<5)|(1u<<4));

  /* Load payload */
  HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_RESET);
  uint8_t cmd = 0xA0; // W_TX_PAYLOAD
  HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
  HAL_SPI_Transmit(&hspi1, (uint8_t*)buf32, 32, 100);
  HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_SET);

  /* Pulse CE */
  HAL_GPIO_WritePin(NRF24_CE_PORT, NRF24_CE_PIN, GPIO_PIN_SET);
  osDelay(1);
  HAL_GPIO_WritePin(NRF24_CE_PORT, NRF24_CE_PIN, GPIO_PIN_RESET);

  /* Wait completion ~5ms */
  uint32_t t0 = HAL_GetTick();
  while ((HAL_GetTick() - t0) < 5)
  {
    uint8_t st = nrf24_ReadReg(STATUS);
    if (st & (1u<<5)) { nrf24_WriteReg(STATUS, (1u<<5)); return 1; }
    if (st & (1u<<4)) { nrf24_WriteReg(STATUS, (1u<<4)); nrf24_flush_tx(); return 0; }
  }

  nrf24_flush_tx();
  return 0;
}
