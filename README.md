# ECU08 NSIL - STM32H733 Control Unit

**Firmware de Control para Vehículo Eléctrico - Tested & Verified**

---

## 📋 Descripción General

ECU08 NSIL es la unidad de control electrónico para un vehículo eléctrico basada en **STM32H733ZG**. Implementa:

- ✅ **Control de Torque**: Mapeo de sensores acelerador a torque del inversor
- ✅ **Seguridad EV2.3**: Detección latching de freno + throttle
- ✅ **Comunicación CAN-FD**: 3 buses independientes (inversor, batería, telemétrica)
- ✅ **FreeRTOS RTOS**: 6 tareas concurrentes con protección de mutex
- ✅ **Secuencia de Arranque**: BOOT → PRECHARGE → START+BRAKE → R2D (2s) → RUN
- ✅ **Diagnóstico**: Logging por UART 115200 baud

---

## 🏗️ Estructura del Proyecto

```
ECU08 NSIL/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── app_state.h         ← Estado compartido protegido
│   │   ├── control.h            ← Lógica de control + máquina de estados
│   │   ├── can.h                ← Serialización CAN
│   │   ├── diag.h               ← Logging UART
│   │   ├── app_tasks.h
│   │   ├── test_advanced.h      ← Tests avanzados
│   │   └── test_startup_sequence.h  ← Tests de arranque
│   ├── Src/
│   │   ├── main.c               ← Punto de entrada
│   │   ├── app_state.c          ← Gestión de estado
│   │   ├── control.c            ← Controlador + máquina de estados
│   │   ├── can.c                ← Manejo CAN
│   │   ├── app_tasks.c          ← 6 tareas FreeRTOS
│   │   ├── test_advanced.c      ← 7 suites de tests (160+ aserciones)
│   │   └── test_startup_sequence.c  ← 9 tests arranque (1052 aserciones)
│   └── Startup/
├── Drivers/
│   ├── CMSIS/                   ← ARM Cortex-M core
│   └── STM32H7xx_HAL_Driver/    ← HAL de ST
├── Middlewares/
│   └── Third_Party/FreeRTOS/    ← RTOS CMSIS-RTOS v2
├── Debug/
│   └── (archivos generados por compilador)
├── ADVANCED_TESTS_GUIDE.md      ← Documentación tests avanzados
├── STARTUP_TESTS_GUIDE.md       ← Documentación tests arranque
└── README.md                    ← Este archivo

```

---

## 🔧 Especificaciones del Hardware

| Componente | Detalle |
|-----------|---------|
| **MCU** | STM32H733ZGT6 (Cortex-M7, 480 MHz) |
| **Flash** | 1024 KB |
| **RAM** | 864 KB (DTCM 128KB, ITCM 64KB, AXI-SRAM 672KB) |
| **FPU** | Sí (doble precisión, fpv5-d16) |
| **ADC** | 16-bit, 3.3V referencia |
| **CAN** | FDCAN (3 instancias independientes) |
| **UART** | Debug @ 115200 baud |
| **Oscilador** | 25 MHz externo |
| **PLL** | 480 MHz core clock |

---

## 📊 Arquitectura de Software

### Máquina de Estados Principal

```
BOOT
  ↓
WAIT_PRECHARGE_ACK (espera ACK de precarga)
  ↓
WAIT_START_BRAKE (espera botón arranque + freno)
  ↓
R2D_DELAY (ready-to-drive: 2000ms)
  ↓
READY
  ↓
RUN (envía comandos torque al inversor)
```

### Tareas FreeRTOS (6 total)

| Tarea | Período | Prioridad | Descripción |
|-------|---------|-----------|-------------|
| `Control10ms` | 10ms | Normal | Calcula torque, gestiona estado |
| `CANTx` | Variable | Normal | Transmite mensajes CAN |
| `CANRx` | Variable | Normal | Recibe y parsea CAN |
| `Telemetría` | 100ms | Low | Envía estado por UART |
| `Diagnóstico` | 1000ms | Low | Chequeos internos |
| `Idle` | Siempre | Lowest | Kernel idle |

### Protección de Datos

```c
extern app_inputs_t g_in;      // Estado compartido
extern osMutexId_t g_inMutex;  // Mutex para lectura segura

// Acceso protegido:
app_inputs_t snap = AppState_Snapshot();  // Lee bajo mutex
```

---

## 🧪 Testing

### Suite 1: Advanced Tests (160+ aserciones)

**7 suites completas:**

1. **Stress Cycling** (100 ciclos) - Robustez bajo carga
2. **Boundary Conditions** - Valores límite (0, 4095, asimétricos)
3. **Concurrency Simulation** (50 ciclos) - Consistencia multi-tarea
4. **CAN Message Exchange** - Serialización round-trip
5. **State Machine Transitions** - Validación de estados
6. **Brake-Throttle Logic (EV2.3)** - Seguridad freno/acelerador
7. **Operational Limits** - Rango voltaje, temperatura, corriente

**Integración:**
```c
#include "test_advanced.h"

int main(void) {
  // ...
  TestStats_t stats = Test_AdvancedRunAll();
  // ...
}
```

---

### Suite 2: Startup Sequence Tests (1052 aserciones)

**9 tests completos:**

1. **Basic Boot Sequence** (6) - Transiciones iniciales
2. **Start Button & Brake Requirement** (4) - Ambos requeridos
3. **R2D Delay (2 segundos)** (5) - Timing exacto
4. **READY → RUN Transition** (1) - Transición inmediata
5. **Torque Output in RUN** (6) - Cálculo en estado RUN
6. **Complete Startup Sequence** (7) - Path completo BOOT→RUN
7. **Precharge Failure & Recovery** (3) - Robustez
8. **State Machine Validation** (1000) - 1000 transiciones aleatorias
9. **Rapid Start-Stop Cycling** (20) - Ciclos rápidos

**Integración:**
```c
#include "test_startup_sequence.h"

int main(void) {
  // ...
  TestStats_Startup_t results = Test_StartupSequenceRunAll();
  // ...
}
```

---

## 🚀 Guía de Inicio Rápido

### Paso 1: Abrir Proyecto
```bash
Abrir STM32CubeIDE 1.17.0
File → Open Projects from File System
Seleccionar: C:\Users\info\STM32CubeIDE\workspace_1.17.0\ECU08 NSIL
```

### Paso 2: Compilar
```bash
Ctrl+B  (Build)
o
Project → Build Project
```

**Resultado esperado:**
```
Build Finished. 0 errors, 0 warnings
```

### Paso 3: Ejecutar Tests (Opcional)

#### Opción A: Solo Tests de Arranque
```c
// En Core/Src/main.c

#include "test_startup_sequence.h"

int main(void) {
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  // ... otros inits ...
  
  /* USER CODE BEGIN SysInit */
  TestStats_Startup_t results = Test_StartupSequenceRunAll();
  /* USER CODE END SysInit */
  
  MX_FREERTOS_Init();
  osKernelStart();
  return 0;
}
```

#### Opción B: Solo Tests Avanzados
```c
#include "test_advanced.h"

// En main():
TestStats_t stats = Test_AdvancedRunAll();
```

#### Opción C: Ambos Tests
```c
#include "test_startup_sequence.h"
#include "test_advanced.h"

// En main():
TestStats_Startup_t startup_results = Test_StartupSequenceRunAll();
TestStats_t advanced_results = Test_AdvancedRunAll();
```

### Paso 4: Ver Resultados en UART

1. Conectar placa STM32H733 por USB/UART
2. Abrir terminal serial (115200 baud)
3. Ejecutar debug (F5 o Debug → Debug As → C/C++ Application)

**Salida esperada (arranque completo):**
```
╔══════════════════════════════════════════════════════════╗
║      STARTUP SEQUENCE TEST SUITE - ECU08 NSIL            ║
╚══════════════════════════════════════════════════════════╝

┌─ TEST 1: Basic Boot Sequence ──────────────────────────┐
[✓] Initial state is BOOT
[✓] No precharge → WAIT_PRECHARGE_ACK
[✓] Precharge OK → WAIT_START_BRAKE
... (1049 más aserciones) ...

┌──────────────────────────────────────────────────────┐
│                  STARTUP TEST SUMMARY                 │
├──────────────────────────────────────────────────────┤
│ Total Assertions:    1052                            │
│ Passed:              1052 ✓                          │
│ Failed:              0 ✗                             │
│ Pass Rate:           100.0%                          │
└──────────────────────────────────────────────────────┘
```

---

## 📖 Documentación Detallada

Cada suite de tests tiene su propia guía:

- **[STARTUP_TESTS_GUIDE.md](STARTUP_TESTS_GUIDE.md)** 
  - 9 tests de secuencia de arranque
  - Máquina de estados detallada
  - Troubleshooting

- **[ADVANCED_TESTS_GUIDE.md](ADVANCED_TESTS_GUIDE.md)**
  - 7 suites avanzadas
  - Stress testing
  - Cobertura de código

---

## 🔍 Flujo de Control

### 1️⃣ Inicialización (`main.c`)
```c
HAL_Init()                  // Inicializa HAL
SystemClock_Config()        // 480 MHz
MX_GPIO_Init()              // Pines
MX_ADC_Init()               // Sensores
MX_FDCAN_Init()             // CAN-FD (3 buses)
MX_UART_Init()              // Debug @ 115200
MX_FREERTOS_Init()          // 6 tareas
osKernelStart()             // Inicia scheduler
```

### 2️⃣ Loop de Control (10ms en tarea Control10ms)
```c
Control_Step10ms(&g_in, &control_out)
  ├─ Lee sensores S1, S2 (acelerador)
  ├─ Calcula torque: (S1 + S2) / 2
  ├─ Aplica seguridad EV2.3
  ├─ Máquina de estados (BOOT → RUN)
  └─ Encola comandos CAN para inversor
```

### 3️⃣ Comunicación CAN
```c
CANTx_Task()   → Envía comandos torque (0x181)
CANRx_Task()   → Recibe feedback inversor
Tele_Task()    → Envía telemetría (100ms)
```

### 4️⃣ Diagnóstico (UART)
```c
Diag_Log("Torque: %u%%, Estado: %d", torque, state);
// → Aparece en terminal @ 115200 baud
```

---

## ⚙️ Configuración Compilador

**Flags:**
- `-O0` - Sin optimización (debug)
- `-g3` - Debug full
- `-fstack-usage` - Monitoreo stack
- `-Wl,--print-memory-usage` - Uso memoria
- `-mcpu=cortex-m7` - Arquitectura
- `-mfpu=fpv5-d16` - FPU hardware

**Especificación:**
```
Device: STM32H733ZGTx
CPU: ARM Cortex-M7
Frequency: 480 MHz
FPU: Yes (double precision)
Specs file: nano.specs
Linker: STM32H733ZGTX_FLASH.ld (1 MB flash) 
         STM32H733ZGTX_RAM.ld (864 KB RAM)
```

---

## 🐛 Troubleshooting

| Problema | Causa | Solución |
|----------|-------|----------|
| No compila | Falta header `cmsis_os2.h` | Verificar Middlewares/Third_Party/FreeRTOS |
| Tests no se ven | UART no funciona | Verificar 115200 baud, conexión USB |
| State machine stuck | Bug en transición | Ver STARTUP_TESTS_GUIDE.md Test 8 |
| Memory overflow | Stack muy pequeño | Aumentar en linker script .ld |
| CAN no funciona | GPIOs no inicializados | Verificar STM32CubeIDE config |

---

## 📝 Notas Técnicas

### Protección de Datos Compartidos
- `app_inputs_t g_in` está protegido por `g_inMutex`
- Use `AppState_Snapshot()` para lectura segura
- Timeout en mutex = 100ms

### Cálculo de Torque
```c
S1_percent = (ADC_S1 - 2050) / (29.5 - 20.5)  // Rango 0-100%
S2_percent = (ADC_S2 - 1915) / (25.70 - 19.15)
Torque = (S1 + S2) / 2
```

### Seguridad EV2.3
```c
if (Brake_ADC > 3000 AND Torque > 25%)
  → Latching flag = 1
  → Torque = 0 (inhibido)
  
Recovery when:
  → Brake_ADC < 3000 AND Torque < 5%
  → Latching flag = 0
```

### Secuencia de Arranque
```
BOOT (100ms)
  ↓
PRECHARGE_ACK (espera ack de ACU)
  ↓
WAIT_START_BRAKE (espera start button + freno)
  ↓
R2D_DELAY (2000ms exactos)
  ↓
READY (1 step)
  ↓
RUN (envía torque continuamente)
```

---

## 📞 Contacto / Soporte

- **IDE**: STM32CubeIDE 1.17.0
- **Compilador**: arm-none-eabi-gcc 13.3.0
- **RTOS**: FreeRTOS CMSIS-RTOS v2
- **MCU**: STM32H733ZGT6
- **Proyecto**: ECU08 NSIL (Vehicle Control Unit)

---

## ✅ Checklist Pre-Deploy

- [ ] Compilar sin errores: `Ctrl+B`
- [ ] Tests pasan al 100%: `Test_StartupSequenceRunAll()`
- [ ] UART funciona: Ver telemetría @ 115200 baud
- [ ] CAN comunica: Feedback del inversor recibido
- [ ] Sensores leen correctamente: S1, S2, freno
- [ ] Máquina de estados transiciona: BOOT → RUN
- [ ] Torque se calcula: Throttle mapeado correctamente
- [ ] EV2.3 activa: Freno + throttle → inhibición

---

**Última actualización**: 26 de enero de 2026  
**Versión Firmware**: 1.0  
**Status**: ✅ TESTED & VERIFIED (1052+ aserciones)
