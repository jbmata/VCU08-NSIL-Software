#include "app.h"
#include "cmsis_os2.h"
#include "main.h"
#include "VCU.h"


extern uint8_t flag_r2d;
extern uint8_t precarga_inv;
extern uint8_t config_inv_lectura_v;
extern int inv_dc_bus_voltage;
extern int state;
extern int s_freno;


extern TIM_HandleTypeDef htim16;
extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_HandleTypeDef hfdcan2;
extern FDCAN_HandleTypeDef hfdcan3;

void App_Init(void)
{
    /* cosas que no dependen de estado */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)buffer_adc, 3);

    HAL_FDCAN_Start(&hfdcan1);
    HAL_FDCAN_Start(&hfdcan2);
    HAL_FDCAN_Start(&hfdcan3);

    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    HAL_FDCAN_ActivateNotification(&hfdcan3, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);

    HAL_TIM_Base_Start_IT(&htim16);
}

/* ===================== */
/* ARRANQUE DEL VEHÍCULO */
/* ===================== */


void Secuencia_arranque(void)
{
    print("BOOT: inicio");

    /* 1. Esperar DC BUS */
    while (config_inv_lectura_v == 0)
    {
        osDelay(50);
    }
    print("BOOT: DC BUS OK");

    /* 2. Precarga */
    while (precarga_inv == 0 && inv_dc_bus_voltage < 300)
    {
        osDelay(50);
    }
    print("BOOT: precarga OK");

    /* 3. Botón + freno */
    print("BOOT: esperando botón");
    while (1)
    {
        if (HAL_GPIO_ReadPin(START_BUTTON_GPIO_Port, START_BUTTON_Pin) &&
            s_freno > 900)
        {
            break;
        }
        osDelay(50);
    }
    print("BOOT: botón OK");

    /* 4. RTDS */
    flag_r2d = 1;
    HAL_GPIO_WritePin(RTDS_GPIO_Port, RTDS_Pin, GPIO_PIN_SET);
    osDelay(2000);
    HAL_GPIO_WritePin(RTDS_GPIO_Port, RTDS_Pin, GPIO_PIN_RESET);

    print("BOOT: RTDS terminado");

    /* 5. Esperar estado READY inversor */
    while (state != 4)
    {
        osDelay(20);
    }

    print("BOOT: inversor READY");
    print("BOOT: fin");
}
