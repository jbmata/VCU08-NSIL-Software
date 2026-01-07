#include "app.h"
#include "main.h"

#include "FreeRTOS.h"
#include "task.h"

#include <string.h>
#include <stdio.h>

/* Drivers / módulos */
//#include "nrf24.h"

/* HAL handles (definidos por Cube en main.c / fdcan.c / adc.c / tim.c / spi.c / usart.c) */
extern ADC_HandleTypeDef   hadc1;
extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_HandleTypeDef hfdcan2;
extern FDCAN_HandleTypeDef hfdcan3;
extern SPI_HandleTypeDef   hspi1;
extern UART_HandleTypeDef  huart2;

/* Variables de tu aplicación (ideal: vehicle_state.h) */
extern volatile uint8_t config_inv_lectura_v;
extern volatile uint8_t precarga_inv;
extern volatile uint8_t boton_arranque;

/* ADC DMA buffer (tu código original) */
extern uint16_t buffer_adc[3];

/* NRF */
#define TEL_CHAN 76
static uint8_t rf_addr[5] = {0xE7,0xE7,0xE7,0xE7,0xE7};

#define CSN_LOW()   HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_RESET)
#define CSN_HIGH()  HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_SET)

static void uart_print(const char *s)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

static void dump_reset_cause(void)
{
    uint32_t pin = __HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) ? 1u : 0u;
    uint32_t bor = __HAL_RCC_GET_FLAG(RCC_FLAG_BORRST) ? 1u : 0u;
    uint32_t por = __HAL_RCC_GET_FLAG(RCC_FLAG_PORRST) ? 1u : 0u;
    uint32_t sft = __HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) ? 1u : 0u;

    char msg[96];
    snprintf(msg, sizeof msg, "[RST] PIN=%lu BOR=%lu POR=%lu SFT=%lu\r\n",
             (unsigned long)pin, (unsigned long)bor,
             (unsigned long)por, (unsigned long)sft);
    uart_print(msg);

    __HAL_RCC_CLEAR_RESET_FLAGS();
}

static void can_start_all(void)
{
    if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) Error_Handler();
    if (HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) Error_Handler();

    if (HAL_FDCAN_Start(&hfdcan2) != HAL_OK) Error_Handler();
    if (HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) Error_Handler();

    if (HAL_FDCAN_Start(&hfdcan3) != HAL_OK) Error_Handler();
    if (HAL_FDCAN_ActivateNotification(&hfdcan3, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) Error_Handler();
}

static void adc_start_dma(void)
{
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)buffer_adc, 3) != HAL_OK)
        Error_Handler();
}

/*
static void nrf24_bringup(void)
{

    vTaskDelay(pdMS_TO_TICKS(5));


    HAL_GPIO_WritePin(NRF24_CE_PORT,  NRF24_CE_PIN,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(NRF24_CSN_PORT, NRF24_CSN_PIN, GPIO_PIN_SET);
    vTaskDelay(pdMS_TO_TICKS(1));


    uint8_t w_cfg[2] = { (uint8_t)(0x20 | 0x00), 0x0B };
    uint8_t r_cfg[2] = { 0x00, 0xFF };
    uint8_t rxw[2] = {0}, rxr[2] = {0};

    CSN_LOW();  HAL_SPI_TransmitReceive(&hspi1, w_cfg, rxw, 2, 100);  CSN_HIGH();
    CSN_LOW();  HAL_SPI_TransmitReceive(&hspi1, r_cfg, rxr, 2, 100);  CSN_HIGH();

    char dbg[96];
    snprintf(dbg, sizeof dbg, "[NRF-POST] st_w=%02X cfg=%02X\r\n", rxw[0], rxr[1]);
    uart_print(dbg);

    NRF24_Init();
    NRF24_TxMode(rf_addr, TEL_CHAN);

    nrf24_WriteReg(DYNPD,   0x00);
    nrf24_WriteReg(FEATURE, 0x00);
    nrf24_WriteReg(RX_PW_P0, 32);

    uart_print("[NRF] ready\r\n");
}

*/

void App_Init(void)
{
    uart_print("\r\n=== BOOT ===\r\n");
    dump_reset_cause();

    /* estado inicial */
    config_inv_lectura_v = 0;
    precarga_inv = 0;
    boton_arranque = 0;

    /* ADC + CAN + NRF */
    adc_start_dma();
    can_start_all();
    nrf24_bringup();

    uart_print("[APP] init done\r\n");
}
