#include "estado_vehiculo.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

void DiagTask(void *arg)
{
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("[HB] Vdc=%d State=%u TorqueOK=%u\n",
               vs.inv_dc_bus_voltage,
               vs.inv_state,
               vs.allow_torque);
    }
}
