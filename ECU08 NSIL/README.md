# ECU08 NSIL - STM32H733 Vehicle Control Unit

**Firmware de Control para Vehículo Eléctrico**

Unidad de control electrónico para vehículos eléctricos basada en STM32H733ZG. Implementa control de torque robusto, seguridad EV2.3, comunicación CAN-FD y FreeRTOS multitarea con protección de datos concurrente.

## Características Principales

- **Control de Torque**: Mapeo automático de sensores acelerador a comandos de torque del inversor
- **Seguridad EV2.3**: Protección latching de freno + acelerador con recuperación controlada
- **Comunicación CAN-FD**: 3 buses independientes (inversor, batería, telemetría)
- **FreeRTOS Multitarea**: 6 tareas concurrentes con mutex para protección de datos compartidos
- **Máquina de Estados**: Secuencia de arranque segura (BOOT → PRECHARGE → START → R2D → RUN)
- **Diagnóstico**: Logging en tiempo real por UART a 115200 baud
- **34 Pruebas Unitarias**: Cobertura completa de lógica crítica

---

## Estructura del Proyecto

```
ECU08 NSIL/
├── Core/
│   ├── Inc/                     # Headers del proyecto
│   │   ├── main.h
│   │   ├── app_state.h          # Estado compartido con mutex
│   │   ├── control.h            # Lógica de control y máquina de estados
│   │   ├── can.h                # Serialización CAN-FD
│   │   ├── diag.h               # Logging UART
│   │   ├── app_tasks.h          # Definiciones de tareas FreeRTOS
│   │   └── *.h
│   ├── Src/
│   │   ├── main.c               # Punto de entrada
│   │   ├── app_state.c          # Gestión de estado compartido
│   │   ├── control.c            # Control de torque + máquina de estados
│   │   ├── can.c                # Procesamiento de mensajes CAN
│   │   ├── app_tasks.c          # 6 tareas FreeRTOS
│   │   ├── telemetry.c          # Construcción de payloads de telemetría
│   │   └── *.c
│   └── Startup/
├── Drivers/                     # Drivers de ST
│   ├── CMSIS/                   # Core ARM Cortex-M
│   └── STM32H7xx_HAL_Driver/    # Hardware Abstraction Layer
├── Middlewares/                 # Stack de middleware
│   └── Third_Party/FreeRTOS/    # FreeRTOS CMSIS-RTOS v2
├── CMakeLists.txt               # Configuración de build
├── *.ld                         # Linker scripts (FLASH/RAM)
└── *.md                         # Documentación
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
  ↓ (100 ms)
WAIT_PRECHARGE_ACK
  ↓ (espera ACK del ACU)
WAIT_START_BRAKE
  ↓ (espera botón de arranque + freno)
R2D_DELAY
  ↓ (2000 ms)
READY
  ↓ (1 ciclo de control)
RUN
  ↓ (envía comandos de torque continuamente)
```

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

## Pruebas Unitarias

El proyecto incluye 34 pruebas unitarias que cubren la lógica crítica del sistema:

- **13 pruebas CAN**: Serialización, parseo y validación de mensajes
- **12 pruebas de Control**: Cálculo de torque, máquina de estados, lógica de seguridad
- **9 pruebas de Telemetría**: Construcción de payloads, empaquetamiento

Estas pruebas pueden ejecutarse en Windows sin hardware usando CMake:

```bash
cd build
ctest --output-on-failure -V
```

El framework Unity proporciona aserciones detalladas y reportes claros.

---

## Inicio Rápido

### 1. Clonar y Abrir en STM32CubeIDE

```bash
git clone <repo-url>
cd ECU08_NSIL
```

Luego abrir en STM32CubeIDE 1.17.0:
- File → Open Projects from File System
- Seleccionar la carpeta del proyecto

### 2. Compilar el Firmware

**Opción A: Desde STM32CubeIDE**
```
Ctrl+B  (Build)
```

**Opción B: Desde terminal (requiere GCC instalado)**
```bash
mkdir -p build
cd build
cmake ..
make
```

Resultado esperado: **0 errores, 0 warnings**

### 3. Ejecutar Pruebas (opcional)

Para ejecutar las 34 pruebas unitarias en Windows:

```bash
cd build
cmake ..
make
ctest --output-on-failure -V
```

### 4. Programar en STM32H733

1. Conectar placa STM32H733 por ST-Link
2. En STM32CubeIDE: Run → Debug As → C/C++ Application
3. El firmware se programa automáticamente
4. Ver salida de depuración en UART (115200 baud)

---

## Documentación

- [ARCHITECTURE_DIAGRAM.md](ARCHITECTURE_DIAGRAM.md) - Diagrama de arquitectura del sistema
- [BOOT_SEQUENCE.md](BOOT_SEQUENCE.md) - Secuencia de arranque detallada
- [INTEGRATION_TESTS.md](INTEGRATION_TESTS.md) - Diseño de pruebas de integración
- [HARDWARE_IN_LOOP_TESTING.md](HARDWARE_IN_LOOP_TESTING.md) - Pruebas de hardware en bucle
- [TESTING_README.md](TESTING_README.md) - Guía de pruebas unitarias
- [QUICK_START.md](QUICK_START.md) - Inicio rápido

---

## Flujo de Ejecución

### Inicialización del Sistema

```c
main()
├─ HAL_Init()                   // Hardware Abstraction Layer
├─ SystemClock_Config()         // Configura 480 MHz
├─ MX_GPIO_Init()               // Inicializa GPIOs
├─ MX_ADC_Init()                // Sensores (S1, S2, freno)
├─ MX_FDCAN_Init()              // 3 buses CAN-FD
├─ MX_UART_Init()               // UART @ 115200 baud
├─ MX_FREERTOS_Init()           // 6 tareas FreeRTOS
└─ osKernelStart()              // Inicia scheduler
```

### Loop de Control (10 ms)

La tarea de control se ejecuta cada 10 ms:

```c
ControlTask() [10 ms]
├─ Lee sensores S1, S2 (acelerador) vía ADC
├─ Control_ComputeTorque()      // Mapea ADC a 0-100%
├─ Aplica lógica de seguridad EV2.3
├─ Máquina de estados (BOOT → RUN)
└─ Encola comando de torque para CAN TX
```

### Comunicación CAN-FD

- **Recepción (CanRx)**: Parsea mensajes de inversor, ACU, dashboard
- **Transmisión (CanTx)**: Envía comandos de torque y estado del sistema
- **Telemetría**: Transmite estado cada 100 ms por UART para diagnóstico

---

## Configuración de Compilación

- **Optimización**: `-O0` (sin optimización para debug)
- **Debug Info**: `-g3` (información completa)
- **Arquitectura**: `-mcpu=cortex-m7` (ARM Cortex-M7)
- **FPU**: `-mfpu=fpv5-d16` (hardware floating-point)
- **Stack**: Monitoreo de uso en tiempo real
- **Memoria**: Linker scripts separados para FLASH (1 MB) y RAM (864 KB)

### Compilador

- **Toolchain**: arm-none-eabi-gcc 13.3.0
- **Sistema Build**: CMake 3.15+
- **IDE**: STM32CubeIDE 1.17.0

---

## Resolución de Problemas

| Problema | Causa | Solución |
|----------|-------|----------|
| No compila | Falta header CMSIS | Verificar `Middlewares/Third_Party/FreeRTOS` |
| Tests no se ejecutan | UART no configurado | Verificar 115200 baud y conexión USB |
| Máquina de estados atascada | Bug en transición | Revisar `BOOT_SEQUENCE.md` |
| Overflow de memoria | Stack insuficiente | Aumentar tamaño en linker script `.ld` |
| CAN no funciona | GPIOs no inicializados | Verificar configuración de pines |

---

## Consideraciones de Seguridad

### Cálculo de Torque

La entrada de torque se calcula a partir de dos sensores de acelerador independientes (S1 y S2):

```
S1_percent = (ADC_S1 - 2050) / (29.5 - 20.5)  // Normalización 0-100%
S2_percent = (ADC_S2 - 1915) / (25.70 - 19.15)
Torque = (S1_percent + S2_percent) / 2

// Umbrales:
if Torque < 10% → 0
if Torque > 90% → 100
```

### Protección EV2.3 (Freno + Acelerador)

Implementa un latch de seguridad cuando se detecta freno + acelerador simultáneamente:

```
if Freno > Threshold AND Acelerador > 25%
  → Torque = 0 (inhibición total)
  → flag_EV_2_3 = LATCHED
  
Recovery solo cuando:
  → Freno < Threshold AND Acelerador < 5%
  → flag_EV_2_3 = 0
```

### Verificación de Plausibilidad (T11.8.9)

Compara diferencia entre sensores para detectar fallos:

```
Desviación = |S1_percent - S2_percent|
if Desviación > 10%
  → flag_T11_8_9 = FAULT
```

### Limitación por Voltaje de Celda

El torque se limita dinámicamente según el voltaje mínimo de celda:

```
if v_celda_min < umbral_critico
  → Torque_limitado = Torque * factor_reduccion
```

---

## Flujo de Desarrollo

1. **Desarrollo**: Escribir código y pruebas unitarias localmente
2. **Testing**: Ejecutar pruebas en Windows con CMake
3. **Validación**: Compilar firmware para STM32H733
4. **Despliegue**: Programar en hardware vía ST-Link
5. **Debugging**: Monitorear salida UART en tiempo real

---

## Información del Proyecto

- **Nombre**: ECU08 NSIL
- **Versión**: 1.0
- **MCU**: STM32H733ZGT6
- **Toolchain**: arm-none-eabi-gcc 13.3.0
- **RTOS**: FreeRTOS CMSIS-RTOS v2
- **IDE**: STM32CubeIDE 1.17.0
- **Última actualización**: 26 de enero de 2026

---

**Copyright (c) 2026**. Todos los derechos reservados.
