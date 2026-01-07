#include "can.h"
#include "estado_vehiculo.h"
#include "VCU.h"
#include "main.h"
QueueHandle_t canRxQueue;
QueueHandle_t canTxQueue;

void CAN_InitRTOS(void)
{
    canRxQueue = xQueueCreate(32, sizeof(CanRxMsg_t));
    canTxQueue = xQueueCreate(32, sizeof(CanTxMsg_t));
}

void CAN_PushRxFromISR(FDCAN_HandleTypeDef *hcan)
{
    CanRxMsg_t msg;
    msg.hcan = hcan;

    if (HAL_FDCAN_GetRxMessage(hcan, FDCAN_RX_FIFO0, &msg.hdr, msg.data) != HAL_OK)
        return;

    BaseType_t hpw = pdFALSE;
    xQueueSendFromISR(canRxQueue, &msg, &hpw);
    portYIELD_FROM_ISR(hpw);
}

void CAN_Send(const CanTxMsg_t *msg)
{
    xQueueSend(canTxQueue, msg, 0);
}

/* ---------- RX TASK ---------- */
void CanRxTask(void *arg)
{
    CanRxMsg_t rx;

    for (;;)
    {
        if (xQueueReceive(canRxQueue, &rx, portMAX_DELAY))
        {
            if (rx.hcan == &hfdcan1) {
                /* INV */
                if (rx.hdr.Identifier == TX_STATE_2)
                    vs.inv_state = rx.data[4] & 0xF;

                if (rx.hdr.Identifier == TX_STATE_7)
                    vs.inv_dc_bus_voltage = (rx.data[3] << 8) | rx.data[2];
            }
            else if (rx.hcan == &hfdcan2) {
                /* ACU */
                if (rx.hdr.Identifier == 0x20)
                    vs.acu_precarga_ok = (rx.data[0] == 0);

                if (rx.hdr.Identifier == 0x12C)
                    vs.v_celda_min = (rx.data[0] << 8) | rx.data[1];

                vs.acu_last_rx_ms = HAL_GetTick();
            }
            else if (rx.hcan == &hfdcan3) {
                /* DRIVER */
                if (rx.hdr.Identifier == 0x101) {
                    vs.s1_accel = (rx.data[0] << 8) | rx.data[1];
                    vs.s2_accel = (rx.data[2] << 8) | rx.data[3];
                }
            }
        }
    }
}

/* ---------- TX TASK ---------- */
void CanTxTask(void *arg)
{
    CanTxMsg_t tx;

    for (;;)
    {
        if (xQueueReceive(canTxQueue, &tx, portMAX_DELAY))
        {
            HAL_FDCAN_AddMessageToTxFifoQ(tx.hcan, &tx.hdr, tx.data);
        }
    }
}
