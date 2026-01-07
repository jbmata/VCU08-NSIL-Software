#include "estado_vehiculo.h"
//#include "nrf24.h"
#include "FreeRTOS.h"
#include "task.h"

void TelemetryTask(void *arg)
{
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        /* aquí reutilizas tu tel_build_packet + NRF24_Transmit */
    }
}
