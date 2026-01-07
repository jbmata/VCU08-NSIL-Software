#pragma once
#include <stdint.h>

typedef struct
{
    /* Inversor */
    uint8_t  inv_state;
    int      inv_dc_bus_voltage;
    int      inv_rpm;
    int      inv_t_motor;
    int      inv_t_igbt;
    int      inv_t_air;
    int      inv_n_actual;
    int      inv_i_actual;

    /* Acumulador */
    uint8_t  acu_precarga_ok;
    uint16_t v_celda_min;
    uint32_t acu_last_rx_ms;

    /* Driver */
    uint16_t s1_accel;
    uint16_t s2_accel;
    uint16_t brake;

    /* Flags */
    uint8_t  allow_torque;
    uint8_t  r2d_active;

} VehicleState_t;

extern VehicleState_t vs;
