#include "control.h"
#include "estado_vehiculo.h"
#include "can.h"
#include "VCU.h"
#include "main.h"

#include "FreeRTOS.h"
#include "task.h"

VehicleState_t vs = {0};

/* ---------- torque ---------- */
static uint16_t setTorque(void)
{
    int torque = (vs.s1_accel + vs.s2_accel) / 2;
    if (!vs.allow_torque) torque = 0;
    return (uint16_t)(~torque + 1);
}

/* ---------- CONTROL TASK ---------- */
void ControlTask(void *arg)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /* Supervisión ACU */
        vs.allow_torque =
            (vs.acu_precarga_ok &&
            (HAL_GetTick() - vs.acu_last_rx_ms < 200));

        /* Enviar DC bus a ACU */
        CanTxMsg_t tx = {0};
        tx.hcan = &hfdcan2;
        tx.hdr.Identifier = ID_dc_bus_voltage;
        tx.hdr.DataLength = FDCAN_DLC_BYTES_2;
        tx.hdr.IdType = FDCAN_EXTENDED_ID;
        tx.data[0] = vs.inv_dc_bus_voltage & 0xFF;
        tx.data[1] = vs.inv_dc_bus_voltage >> 8;
        CAN_Send(&tx);

        /* Control inversor */
        if (vs.inv_state == 4 || vs.inv_state == 6) {
            uint16_t tq = setTorque();
            tx.hcan = &hfdcan1;
            tx.hdr.Identifier = 0x362;
            tx.hdr.DataLength = FDCAN_DLC_BYTES_4;
            tx.hdr.IdType = FDCAN_STANDARD_ID;
            tx.data[2] = tq & 0xFF;
            tx.data[3] = tq >> 8;
            CAN_Send(&tx);
        }
    }
}

/* ---------- TIM16 ISR ---------- */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim16)
    {
        BaseType_t hpw = pdFALSE;
        vTaskNotifyGiveFromISR(controlTaskHandle, &hpw);
        portYIELD_FROM_ISR(hpw);
    }
}
