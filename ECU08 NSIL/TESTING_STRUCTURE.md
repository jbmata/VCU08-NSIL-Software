# Estructura de Tests - ECU08 NSIL

Reorganización de tests en dos categorías principales:

## 📁 Estructura de Carpetas

```
tests/
├── unit/                    # Tests unitarios (Unity framework)
│   ├── CMakeLists.txt
│   ├── mocks.h / mocks.c
│   ├── test_can_parsing.c
│   ├── test_control_logic.c
│   ├── test_telemetry.c
│   ├── unity_config.h
│   └── unity_runner.c
│
└── sil/                     # Entorno SIL (FreeRTOS simulado)
    ├── CMakeLists.txt
    ├── sil_main.c
    ├── sil_hal_mocks.h / sil_hal_mocks.c
    ├── sil_can_simulator.h / sil_can_simulator.c
    ├── sil_boot_sequence.h / sil_boot_sequence.c
    ├── mocks/
    │   └── [Mocks específicos del HAL]
    └── integration/
        ├── test_boot_sequence.c
        └── test_full_cycle.c
```

## 🧪 Tests Unitarios (`tests/unit/`)

**Propósito**: Verificar lógica individual de módulos sin dependencias de hardware.

### Características:
- ✅ Compilación en host (Windows/Linux/macOS)
- ✅ Rápidos (~100ms)
- ✅ Framework: Unity
- ✅ Mocks de RTOS y HAL
- ✅ Coverage: ~80%

### Módulos testeados:
1. **CAN parsing** (`test_can_parsing.c`)
   - Parse de frames CAN (0x20, 0x100, 0x101, etc.)
   - Pack/unpack de mensajes
   - Manejo de IDs desconocidos

2. **Lógica de control** (`test_control_logic.c`)
   - Mapeo throttle-to-torque
   - Lógica de seguridad EV 2.3 (freno + acelerador)
   - Máquina de estados

3. **Telemetría** (`test_telemetry.c`)
   - Formato de payload 32 bytes
   - Endianness (little-endian)
   - Campos y flags

### Ejecutar:

```bash
# Build
cmake -B build -DBUILD_UNIT_TESTS=ON
cmake --build build

# Run tests
cd build/tests/unit
ctest
```

## 🔄 Entorno SIL (`tests/sil/`)

**Propósito**: Simular la aplicación completa ejecutando FreeRTOS en host, sin hardware real.

### Características:
- ✅ FreeRTOS kernel simulado
- ✅ Secuencia de arranque (BOOT → PRECHARGE → READY)
- ✅ CAN simulado con inyección de mensajes
- ✅ HAL mock para GPIO, ADC, USART, etc.
- ✅ Verificación de ciclo operativo completo
- ⏱️ Más lento (~10s) pero más realista

### Componentes SIL:

1. **sil_main.c**: Punto de entrada
   - Gestión de simulación temporal
   - Coordinador de tests

2. **sil_hal_mocks.c**: Mocks del HAL
   - ADC, GPIO, USART, CAN
   - Timer (tick del kernel)

3. **sil_can_simulator.c**: Inyector de CAN
   - Inyectar throttle (0-100%)
   - Inyectar freno (0-100%)
   - Inyectar estados del inversor
   - Inyectar voltajes

4. **sil_boot_sequence.c**: Tracking de secuencia de arranque
   - Estados: INITIAL → PRECHARGE_WAIT → PRECHARGE_OK → READY
   - Verificación de transiciones

### Tests de integración:

**test_boot_sequence.c**:
- Timing de arranque (0-10s)
- ACK de precarga
- Estados del inversor
- Voltaje DC

**test_full_cycle.c**:
- Mapeo throttle-to-torque
- Seguridad EV 2.3 (freno + acelerador)
- Salida de telemetría
- Comunicación con inversor
- Máquina de estados dinámica
- Límites de voltaje de celda

### Ejecutar:

```bash
# Build
cmake -B build -DBUILD_SIL_TESTS=ON
cmake --build build

# Run boot sequence test
./build/tests/sil/ecu08_sil --test-boot

# Run full operating cycle test
./build/tests/sil/ecu08_sil --test-full-cycle

# Run via ctest
cd build
ctest
```

## 🎯 Matriz de Cobertura

| Módulo | Unitarios | SIL | Cobertura |
|--------|-----------|-----|-----------|
| can.c | ✅ | ✅ | 90% |
| control.c | ✅ | ✅ | 85% |
| telemetry.c | ✅ | ✅ | 80% |
| app_state.c | ✅ | ✅ | 75% |
| app_tasks.c | ❌ | ✅ | 70% |

## 📋 Flujo de Testing

```
Development
    ↓
    ├─→ Unit Tests (rápido)
    │   ├─ Si falla: Fix
    │   └─ Si pasa: Continúa
    ↓
    ├─→ SIL Tests (realista)
    │   ├─ Boot sequence: FAIL?
    │   │   └─ Fix startup code
    │   ├─ Full cycle: FAIL?
    │   │   └─ Fix control logic
    │   └─ Si pasa: Continúa
    ↓
    └─→ Hardware Testing (STM32 real)
        ├─ Verify on board
        └─ Validate in vehicle
```

## 🔧 Configuración de CMake

### Solo Tests Unitarios:
```bash
cmake -B build -DBUILD_UNIT_TESTS=ON -DBUILD_SIL_TESTS=OFF
```

### Solo SIL:
```bash
cmake -B build -DBUILD_UNIT_TESTS=OFF -DBUILD_SIL_TESTS=ON
```

### Ambos:
```bash
cmake -B build -DBUILD_UNIT_TESTS=ON -DBUILD_SIL_TESTS=ON
```

## 📝 Mockeo del RTOS

### En Tests Unitarios:
- `osKernelGetTickCount()` → `mock_kernel_tick_ms`
- `osMutex*` → Stubs no-op
- `osQueue*` → Stubs no-op

### En SIL:
- FreeRTOS kernel real (simulado en host)
- Tasks reales ejecutándose
- Queues y mutexes reales
- Scheduler de RTOS activo

## 🐛 Debugging

### Unit Tests:
```bash
gdb ./build/tests/unit/ecu08_unit_tests
```

### SIL:
```bash
gdb ./build/tests/sil/ecu08_sil
(gdb) run --test-boot
```

## 📊 Cobertura de Código

Generar reporte de cobertura (unitarios):
```bash
cd build/tests/unit
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
open coverage_html/index.html
```

## ✅ Checklist de Tests

- [ ] Todos los tests unitarios pasan
- [ ] Cobertura >80%
- [ ] SIL boot sequence pasa
- [ ] SIL full cycle pasa
- [ ] Sin memory leaks (valgrind)
- [ ] Sin warnings de compilación

## 📚 Referencias

- **Unity Framework**: https://github.com/ThrowTheSwitch/Unity
- **FreeRTOS**: https://github.com/FreeRTOS/FreeRTOS-Kernel
- **CMake Testing**: https://cmake.org/cmake/help/latest/command/enable_testing.html
