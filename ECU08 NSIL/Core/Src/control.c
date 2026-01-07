#include "control.h"
#include "cmsis_os2.h"
#include "main.h"
#include "VCU.h"
#include <string.h>
#include <stdio.h>

/* Task handle CMSIS creado por freertos.c */
extern osThreadId_t ControlTaskHandle;

/* HAL handles */
extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_HandleTypeDef hfdcan2;

/* Tus globals (de tu código) */
extern int inv_dc_bus_voltage;
extern int state;
extern uint8_t flag_r2d;
extern int precarga_inv;
extern int config_inv_lectura_v;
extern int precharge_button;

extern FDCAN_TxHeaderTypeDef TxHeader_Inv;
extern FDCAN_TxHeaderTypeDef TxHeader_Acu;
extern uint8_t TxData_Inv[8];
extern uint8_t TxData_Acu[8];

extern uint16_t real_torque;
extern uint8_t  byte_torque_1;
extern uint8_t  byte_torque_2;

extern volatile uint32_t tel_irq_cnt;
extern uint32_t tel_tick;

/* Tu función existente */
extern uint16_t setTorque(void);

/* ========= ISR: solo señaliza el tick ========= */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM16)
  {
    /* Mantén tus contadores aquí si quieres (son muy ligeros) */
    tel_irq_cnt++;
    tel_tick += 10; /* si TIM16 = 10ms */

    /* despierta la tarea de control */
    (void)osThreadFlagsSet(ControlTaskHandle, CTRL_TICK_FLAG);
  }
}

void Control_Init(void)
{
  /* Si necesitas algo al inicio del control, ponlo aquí */
}

/* ========= Paso de control: aquí pones lo que antes hacías en el ISR ========= */
void Control_Step(void)
{
  /* (A) Reenvío DC_BUS_VOLTAGE al AMS por CAN_ACU (tu lógica original) */
  TxHeader_Acu.Identifier = ID_dc_bus_voltage;
  TxHeader_Acu.DataLength = 2;
  TxHeader_Acu.IdType     = FDCAN_EXTENDED_ID;
  TxHeader_Acu.FDFormat   = FDCAN_CLASSIC_CAN;
  TxHeader_Acu.TxFrameType= FDCAN_DATA_FRAME;

  TxData_Acu[0] = inv_dc_bus_voltage & 0xFF;
  TxData_Acu[1] = (inv_dc_bus_voltage >> 8) & 0xFF;
  (void)HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader_Acu, TxData_Acu);

  /* (B) Botón precarga reenviado (tu lógica original) */
  precharge_button = HAL_GPIO_ReadPin(START_BUTTON_GPIO_Port, START_BUTTON_Pin);

  TxHeader_Acu.Identifier = 0x600;
  TxHeader_Acu.DataLength = 2;
  TxHeader_Acu.IdType     = FDCAN_EXTENDED_ID;
  TxHeader_Acu.FDFormat   = FDCAN_CLASSIC_CAN;
  TxHeader_Acu.TxFrameType= FDCAN_DATA_FRAME;
  TxData_Acu[0] = (uint8_t)precharge_button;
  (void)HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader_Acu, TxData_Acu);

  /* (C) CONTROL INVERSOR (tu lógica original, sacada del ISR) */
#if !CALIBRATION
  if ((state == 4 || state == 6) && flag_r2d == 1)
  {
    TxHeader_Inv.Identifier = RX_SETPOINT_1;
    TxHeader_Inv.DataLength = 3;
    TxHeader_Inv.IdType     = FDCAN_STANDARD_ID;

    TxData_Inv[0] = 0x0;
    TxData_Inv[1] = 0x0;
    TxData_Inv[2] = 0x6;
    (void)HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader_Inv, TxData_Inv);
  }

  if (flag_r2d == 1)
  {
    switch (state)
    {
      case 3: /* standby -> pedir READY */
      case 0:
      {
        TxHeader_Inv.Identifier = RX_SETPOINT_1;
        TxHeader_Inv.DataLength = 3;
        TxHeader_Inv.IdType     = FDCAN_STANDARD_ID;

        TxData_Inv[0] = 0x0;
        TxData_Inv[1] = 0x0;
        TxData_Inv[2] = 0x4;
        (void)HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader_Inv, TxData_Inv);
      } break;

      case 4: /* ready: torque = 0 (tu lógica original) */
      {
        TxHeader_Inv.Identifier = 0x362;
        TxHeader_Inv.DataLength = 4;

        real_torque = 0;
        TxData_Inv[0] = 0x0;
        TxData_Inv[1] = 0x0;
        TxData_Inv[2] = (uint8_t)real_torque;
        TxData_Inv[3] = 0x0;
        (void)HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader_Inv, TxData_Inv);
      } break;

      case 6: /* torque */
      {
        real_torque   = setTorque();
        byte_torque_1 = real_torque & 0xFF;
        byte_torque_2 = (real_torque >> 8) & 0xFF;

        TxHeader_Inv.Identifier = 0x362;
        TxHeader_Inv.DataLength = 4;

        TxData_Inv[0] = 0x00;
        TxData_Inv[1] = 0x00;
        TxData_Inv[2] = byte_torque_1;
        TxData_Inv[3] = byte_torque_2;
        (void)HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader_Inv, TxData_Inv);
      } break;

      default:
        break;
    }
  }
#endif
}
