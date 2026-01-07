#pragma once
#include "main.h"
#include "FreeRTOS.h"
#include "queue.h"

typedef struct {
    FDCAN_HandleTypeDef *hcan;
    FDCAN_RxHeaderTypeDef hdr;
    uint8_t data[8];
} CanRxMsg_t;

typedef struct {
    FDCAN_HandleTypeDef *hcan;
    FDCAN_TxHeaderTypeDef hdr;
    uint8_t data[8];
} CanTxMsg_t;

extern QueueHandle_t canRxQueue;
extern QueueHandle_t canTxQueue;

void CAN_InitRTOS(void);
void CAN_Send(const CanTxMsg_t *msg);

/* ISR hook */
void CAN_PushRxFromISR(FDCAN_HandleTypeDef *hcan);
