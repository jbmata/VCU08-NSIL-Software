# ECU08 NSIL - STM32H733 Vehicle Control Unit

**Firmware de Control para Vehículo Eléctrico de Competición (Formula Student EV)**

Unidad de control electrónico basada en STM32H733ZG. Implementa control de torque robusto, seguridad EV2.3, comunicación CAN-FD con 3 buses independientes y FreeRTOS multitarea con protección de datos concurrente. Incluye infraestructura completa de pruebas SIL (Software-In-The-Loop) ejecutable en PC sin hardware.

## Características Principales

- **Control de Torque**: Mapeo calibrado ADC → torque con sensores duales S1/S2 (fórmula real)
- **Seguridad EV2.3**: Latch freno+acelerador con recuperación controlada (normativa Formula Student)
- **Comunicación CAN-FD**: 3 buses FDCAN independientes (inversor BAMOCAR, ACU batería, telemetría)
- **FreeRTOS Multitarea**: Tareas concurrentes con mutex protegiendo el estado compartido `g_in`
- **FSM de Arranque**: BOOT → PRECHARGE → WAIT_START_BRAKE → R2D_DELAY(2s) → READY → RUN
- **Diagnóstico**: Logging en tiempo real por USART10 a 115200 baud
- **93 Tests de Integración**: 10 suites SIL ejecutables en PC sin hardware (100% PASS)

---

## Estructura del Proyecto

```
ECU08 NSIL/
├── Core/
│   ├── Inc/                     # Headers del proyecto
│   │   ├── main.h               # Punto de entrada HAL
│   │   ├── app_state.h          # Estado compartido con mutex
│   │   ├── control.h            # Lógica de control y FSM
│   │   ├── can.h                # Serialización CAN-FD
│   │   ├── diag.h               # Logging UART
│   │   ├── test_integration.h   # API de tests de integración
│   │   └── *.h
│   ├── Src/
│   │   ├── main.c               # Punto de entrada
│   │   ├── freertos.c           # Inicialización de tareas, colas y mutex
│   │   ├── app_state.c          # Gestión del estado compartido
│   │   ├── control.c            # Torque + FSM de arranque
│   │   ├── can.c                # Parseo y serialización CAN
│   │   ├── telemetry.c          # Payloads de telemetría
│   │   ├── test_integration.c   # 10 suites de tests de integración
│   │   └── *.c
│   └── Startup/
├── Drivers/                     # Drivers ST (CMSIS + HAL)
├── Middlewares/
│   └── Third_Party/FreeRTOS/    # FreeRTOS CMSIS-RTOS v2
├── tests/
│   ├── unit/                    # Tests unitarios (Unity framework)
│   └── sil/                     # Infraestructura SIL para PC
│       ├── mocks/               # Mocks de HAL, RTOS y FDCAN
│       │   ├── cmsis_os2.h      # Tipos CMSIS-RTOS v2 (sin FreeRTOS real)
│       │   ├── cmsis_os2_impl.c # Colas ring-buffer, mutex no-op, tick simulado
│       │   ├── main.h           # Tipos HAL/FDCAN sin STM32 HAL real
│       │   ├── hal_impl.c       # Stubs FDCAN, hfdcan1/2/3
│       │   └── diag_sil.c       # Diag_Log → stdout + archivo .log
│       ├── build/               # Ejecutable compilado (ecu08_sil.exe)
│       ├── results/             # Logs de ejecución de tests
│       ├── integration/         # Escenarios SIL adicionales
│       ├── sil_main.c           # Punto de entrada SIL
│       └── *.c / *.h
├── CMakeLists.txt               # Configuración (requiere CMake instalado)
├── STM32H733ZGTX_FLASH.ld       # Linker script FLASH
├── STM32H733ZGTX_RAM.ld         # Linker script RAM
├── INTEGRATION_TESTS_EXPLAINED.md  # Documentación completa de tests
└── AGENT_ANALYSIS_PROMPT.md     # Prompt para análisis automático del sistema
```

---

## Especificaciones del Hardware

| Componente | Especificación |
|-----------|-----------------|
| **Microcontrolador** | STM32H733ZGT6 |
| **Núcleo** | ARM Cortex-M7 @ 480 MHz |
| **Memoria Flash** | 1024 KB |
| **Memoria RAM** | 864 KB (DTCM 128KB, ITCM 64KB, AXI-SRAM 672KB) |
| **Unidad FPU** | Sí (doble precisión, FPv5-d16) |
| **ADC** | 16-bit, referencia 3.3V |
| **CAN-FD** | 3 instancias FDCAN independientes |
| **UART** | Depuración a 115200 baud |
| **Oscilador Externo** | 25 MHz |
| **Reloj Principal** | 480 MHz (vía PLL) |

---

## Arquitectura del Software

### Máquina de Estados del Control

```
BOOT
  ↓ (primera llamada a Control_Step10ms)
WAIT_PRECHARGE_ACK
  ↓ ok_precarga = 1  (CAN ID 0x020, byte0 = 0x01 desde ACU)
WAIT_START_BRAKE
  ↓ boton_arranque = 1  AND  s_freno > 3000
R2D_DELAY
  ↓ 2000 ms  (Ready-to-Drive delay, normativa EV)
READY
  ↓ (1 ciclo de transición)
RUN
  ↓ genera trama CAN 0x181 con torque_pct cada 10 ms
```

> **Importante**: `torque_pct > 0` y tramas CAN al inversor **solo** se producen en estado RUN.

### Tareas FreeRTOS

| Tarea | Período | Prioridad | Función |
|-------|---------|-----------|---------|
| **Control** | 10 ms | Alta | Calcula torque y gestiona máquina de estados |
| **CAN RX** | Variable | Alta | Recibe y parsea mensajes CAN |
| **CAN TX** | Variable | Normal | Transmite comandos de torque |
| **Telemetría** | 100 ms | Normal | Envía estado por UART |
| **Diagnóstico** | 1000 ms | Baja | Chequeos internos del sistema |
| **Idle** | Continuo | Mínima | Kernel idle del scheduler |

### Protección de Datos Compartidos

El estado de la aplicación (`app_inputs_t`) está protegido por mutex para garantizar consistencia en acceso multitarea:

```c
// Lectura segura del estado
app_inputs_t snapshot = AppState_Snapshot();  // Usa mutex internamente
```

---

## Tests de Integración SIL (Software-In-The-Loop)

El proyecto incluye **93 tests de integración** en 10 suites ejecutables en PC sin hardware STM32.

### Resultado última ejecución: 93/93 PASS (100%) — 23-feb-2026

| Suite | Tests | Qué verifica |
|-------|-------|--------------|
| S1 – Mutex / AppState | 9 | Sincronización mutex, snapshot coherente |
| S2 – FSM Inversor BAMOCAR | 7 | Parseo estados 2→7, voltaje DC bus |
| S3 – Secuencia arranque | 9 | BOOT→RUN completo, sin torque antes de RUN |
| S4 – CAN RX parsing | 6 | Todos los IDs del protocolo |
| S5 – CAN TX pack/unpack | 23 | Serialización, FIFO ordering, 8 bytes |
| S6 – Control logic ADC reales | 8 | Conversión ADC→torque con valores calibrados |
| S7 – Seguridad EV2.3 | 10 | Latch freno+acelerador, liberación controlada |
| S8 – Pipeline completo | 9 | sensor→control→CAN→telemetría |
| S9 – Concurrencia FreeRTOS | 6 | Sin race conditions, colas independientes |
| S10 – Estrés / límites | 6 | 100 ciclos, cola llena, ADC 0 y 4095 |

Los tests usan el mismo API FreeRTOS (CMSIS-RTOS v2) que el firmware real, con mocks ligeros solo para HAL/FDCAN. Ver [INTEGRATION_TESTS_EXPLAINED.md](INTEGRATION_TESTS_EXPLAINED.md) para descripción detallada.

### Compilar y ejecutar en PC (requiere TDM-GCC o MinGW)

```powershell
$base = "...\ECU08 NSIL"; $sil = "$base\tests\sil"
gcc -Wall -Wextra -Wno-unused-parameter `
    -DSIL_BUILD=1 -DTEST_MODE_SIL=1 `
    -I"$sil\mocks" -I"$sil" -I"$base\Core\Inc" `
    "$base\Core\Src\app_state.c" `
    "$base\Core\Src\can.c" `
    "$base\Core\Src\control.c" `
    "$base\Core\Src\telemetry.c" `
    "$base\Core\Src\test_integration.c" `
    "$sil\mocks\cmsis_os2_impl.c" `
    "$sil\mocks\hal_impl.c" `
    "$sil\mocks\diag_sil.c" `
    "$sil\sil_main.c" `
    "$sil\sil_hal_mocks.c" `
    "$sil\sil_can_simulator.c" `
    "$sil\sil_boot_sequence.c" `
    "$sil\sil_results.c" `
    "$sil\integration\test_boot_sequence.c" `
    "$sil\integration\test_full_cycle.c" `
    -o "$sil\build\ecu08_sil.exe" -lm

.\tests\sil\build\ecu08_sil.exe --test-integration
```

El informe completo se guarda en `tests/sil/results/integration_test.log`.

---

## Inicio Rápido

### 1. Abrir en STM32CubeIDE

Abrir STM32CubeIDE 1.17.0:
- File → Open Projects from File System
- Seleccionar la carpeta del proyecto

### 2. Compilar el Firmware (hardware)

Desde STM32CubeIDE: `Ctrl+B`

Resultado esperado: **0 errores, 0 warnings**

### 3. Ejecutar Tests SIL en PC

Requisito: TDM-GCC-64 o MinGW instalado en `C:\TDM-GCC-64\bin\gcc.exe`

```powershell
# Desde la raíz del proyecto:
$base = "$PWD"; $sil = "$base\tests\sil"
New-Item -ItemType Directory -Force -Path "$sil\build" | Out-Null
gcc -DSIL_BUILD=1 -DTEST_MODE_SIL=1 -I"$sil\mocks" -I"$sil" -I"$base\Core\Inc" `
    "$base\Core\Src\app_state.c" "$base\Core\Src\can.c" "$base\Core\Src\control.c" `
    "$base\Core\Src\telemetry.c" "$base\Core\Src\test_integration.c" `
    "$sil\mocks\cmsis_os2_impl.c" "$sil\mocks\hal_impl.c" "$sil\mocks\diag_sil.c" `
    "$sil\sil_main.c" "$sil\sil_hal_mocks.c" "$sil\sil_can_simulator.c" `
    "$sil\sil_boot_sequence.c" "$sil\sil_results.c" `
    "$sil\integration\test_boot_sequence.c" "$sil\integration\test_full_cycle.c" `
    -o "$sil\build\ecu08_sil.exe" -lm
& "$sil\build\ecu08_sil.exe" --test-integration
```

### 4. Programar en STM32H733

1. Conectar placa STM32H733 por ST-Link
2. STM32CubeIDE: Run → Debug As → C/C++ Application
3. Ver salida de diagnóstico: USART10 @ 115200 baud

---

## Documentación

| Archivo | Contenido |
|---------|-----------|
| [INTEGRATION_TESTS_EXPLAINED.md](INTEGRATION_TESTS_EXPLAINED.md) | Descripción detallada de las 10 suites, lógica de control y FreeRTOS |
| [AGENT_ANALYSIS_PROMPT.md](AGENT_ANALYSIS_PROMPT.md) | Prompt para análisis automático del sistema por un agente LLM |

---

## Flujo de Ejecución

### Inicialización del Sistema

```c
main()
├─ HAL_Init()                   // Hardware Abstraction Layer
├─ SystemClock_Config()         // Configura 480 MHz
├─ MX_GPIO_Init()               // GPIOs
├─ MX_ADC_Init()                // Sensores ADC (S1, S2, freno)
├─ MX_FDCAN_Init()              // 3 buses CAN-FD
├─ MX_USART_Init()              // USART10 @ 115200 baud
├─ MX_FREERTOS_Init()           // Tareas + colas + mutex
└─ osKernelStart()              // Inicia scheduler FreeRTOS
```

### Loop de Control (10 ms)

```c
ControlTask() [10 ms]
├─ AppState_Snapshot(&in)        // Copia atómica bajo mutex
├─ Control_ComputeTorque(&in)    // ADC → torque 0-100%
├─ Aplica regla seguridad EV2.3  // Latch si freno + acelerador
├─ Control_Step10ms()            // Avanza FSM de arranque
└─ osMessageQueuePut(canTx, ...)  // Encola trama CAN al inversor
```

### Protocolo CAN

| ID (hex) | Bus | Dir | Descripción |
|----------|-----|-----|-------------|
| `0x181` | FDCAN1 (INV) | TX | Comando torque ECU→BAMOCAR |
| `0x201` | FDCAN1 (INV) | RX | Estado/telemetría BAMOCAR→ECU |
| `0x461`–`0x466` | FDCAN1 (INV) | RX | Estados FSM inversor (2→7) |
| `0x020` | FDCAN2 (ACU) | RX | ACK precarga batería |
| `0x101` | FDCAN3 (DASH) | RX | Sensor S1 acelerador (little-endian) |
| `0x102` | FDCAN3 (DASH) | RX | Sensor S2 acelerador (little-endian) |
| `0x103` | FDCAN3 (DASH) | RX | Sensor freno (little-endian) |
| `0x12C` | FDCAN2 (ACU) | RX | Tensión mínima de celda |

---

## Configuración de Compilación

### Firmware (STM32H733)

- **Toolchain**: arm-none-eabi-gcc 13.3.0 (STM32CubeIDE 1.17.0)
- **Optimización**: `-O0 -g3` (debug)
- **Arquitectura**: `-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard`
- **Linker**: scripts separados FLASH (1 MB) y RAM (864 KB)

### Tests SIL (PC)

- **Compilador**: TDM-GCC-64 10.3.0 (`C:\TDM-GCC-64\bin\gcc.exe`)
- **Flags**: `-DSIL_BUILD=1 -DTEST_MODE_SIL=1`
- **Sin CMake**: compilación directa con GCC (ver sección Tests SIL)
- **Salida**: `tests/sil/build/ecu08_sil.exe`

---

## Resolución de Problemas

| Problema | Causa | Solución |
|----------|-------|----------|
| Build SIL falla: `stm32h7xx_hal.h not found` | `can.h` incluye `main.h` sin guarda | Verificar `#ifdef SIL_BUILD` en `Core/Inc/can.h` |
| Tests SIL fallan en S3 | FSM no alcanza RUN | Verificar que `Control_Step10ms` se llama 3 veces tras R2D_DELAY |
| `torque_pct > 0` antes de precarga | Bug: asignación fuera del case RUN | Verificar que `out->torque_pct` solo se asigna en `CTRL_ST_RUN` |
| `Diag_Log` no produce salida en SIL | Weak no-op activo | Verificar que `tests/sil/mocks/diag_sil.c` está en el build |
| Firmware no arranca | `g_inMutex` NULL | Verificar `g_inMutex = osMutexNew(NULL)` en `freertos.c` |
| CAN no funciona en hardware | GPIOs no inicializados | Verificar configuración de pines FDCAN en `.ioc` |

---

## Consideraciones de Seguridad

### Calibración de Sensores ADC

```
S1: s1_pct = (ADC_S1 - 2050) / 9.0      → 0% en raw=2050, 100% en raw=2950
S2: s2_pct = (ADC_S2 - 1915) / 6.55     → 0% en raw=1915, 100% en raw=2570

torque = promedio(s1_pct, s2_pct)  si ambos > 8%  (zona muerta)
torque = 0   si torque calculado < 10%
torque = 100 si torque calculado > 90%   (clamp)
```

### Protección EV2.3 (latch freno + acelerador)

```
ACTIVACIÓN:  s_freno > 3000  AND  torque > 25%  → lat_ev23 = 1, torque = 0
LIBERACIÓN:  s_freno < 3000  AND  torque < 5%   → lat_ev23 = 0
```

El latch persiste aunque se suelte el freno si el acelerador sigue por encima del 5%.

### Verificación de Plausibilidad (T11.8.9, pendiente)

Placeholder implementado. La lógica completa de comparación S1 vs S2 (Δ > 10%) queda pendiente de implementar en `Control_ComputeTorque`.

---

## Flujo de Desarrollo

1. **Desarrollo**: Modificar código en STM32CubeIDE
2. **Testing SIL**: Compilar con GCC y ejecutar `ecu08_sil.exe --test-integration` en PC
3. **Revisar log**: `tests/sil/results/integration_test.log`
4. **Validación firmware**: Compilar con arm-none-eabi-gcc para STM32H733
5. **HIL**: Programar en hardware y monitorear USART10 @ 115200 baud

---

## Información del Proyecto

| Campo | Valor |
|-------|-------|
| **Nombre** | ECU08 NSIL |
| **MCU** | STM32H733ZGT6 |
| **Toolchain firmware** | arm-none-eabi-gcc 13.3.0 |
| **Toolchain SIL** | TDM-GCC-64 10.3.0 |
| **RTOS** | FreeRTOS CMSIS-RTOS v2 |
| **IDE** | STM32CubeIDE 1.17.0 |
| **Tests** | 93/93 PASS (SIL, 10 suites) |
| **Última actualización** | 23 de febrero de 2026 |

---

**Copyright (c) 2026**. Todos los derechos reservados.
