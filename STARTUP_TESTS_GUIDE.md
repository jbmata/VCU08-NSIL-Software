# Test Suite de Secuencia de Arranque - ECU08 NSIL

## Descripción General

Suite completa de **9 tests** para validar la secuencia de arranque del ECU08:

```
BOOT → PRECHARGE_ACK → WAIT_START_BRAKE → R2D_DELAY (2s) → READY → RUN
```

## 9 Tests Incluidos

### 1. **Basic Boot Sequence** (6 aserciones)
Verifica la transición inicial desde BOOT.

**Estados probados:**
- ✓ BOOT es el estado inicial
- ✓ Sin ACK de precarga → WAIT_PRECHARGE_ACK
- ✓ Con precarga OK → WAIT_START_BRAKE

---

### 2. **Start Button & Brake Requirement** (4 aserciones)
Valida que requiere AMBOS: botón de arranque + freno presionado.

**Condiciones:**
- ✓ Solo botón (sin freno) → RECHAZADO
- ✓ Solo freno (sin botón) → RECHAZADO
- ✓ Ambos → R2D_DELAY

---

### 3. **R2D Delay (2 segundos)** (5 aserciones)
Verifica el delay Ready-To-Drive de exactamente 2000ms.

**Validaciones:**
- ✓ Timer inicia en 0ms
- ✓ A 100ms aún en R2D_DELAY
- ✓ A 1500ms aún en R2D_DELAY
- ✓ Después de 2000ms → READY

---

### 4. **READY → RUN Transition** (1 aserción)
Transición inmediata desde READY a RUN.

---

### 5. **Torque Output in RUN State** (6 aserciones)
Verifica cálculo de torque en estado RUN.

**Casos:**
- ✓ 0% throttle (sensores mín) → 0% torque
- ✓ 50% throttle → ~50% torque
- ✓ 100% throttle → 100% torque
- ✓ Tramas CAN se envían continuamente

---

### 6. **Complete Startup Sequence** (7 aserciones)
Path completo: BOOT → PRECHARGE_ACK → WAIT_START_BRAKE → R2D → READY → RUN

**Validaciones:**
- ✓ Fase 1: Espera precarga
- ✓ Fase 2: Precarga llega
- ✓ Fase 3: Arranque + freno detectado
- ✓ Fase 4: Delay R2D completado
- ✓ Fase 5: Estado RUN alcanzado
- ✓ Fase 6: Torque activo

---

### 7. **Precharge Failure & Recovery** (3 aserciones)
Manejo de falla de precarga y recuperación.

**Escenario:**
- Precarga falla inicialmente
- Se recupera después de cierto tiempo
- Sistema continúa arranque normalmente

---

### 8. **State Machine Validation** (1000 aserciones)
Simula 1000 transiciones pseudoaleatorias.

**Garantías:**
- ✓ TODOS los estados permanecen en rango válido
- ✓ No hay estados corruptos o inválidos
- ✓ 80% precharge, 30% start button, freno variable

---

### 9. **Rapid Start-Stop Cycling** (20 aserciones)
Prueba robustez bajo ciclos rápidos.

**Ejecución:**
- 20 ciclos de arranque completo
- Transiciones rápidas sin fallos
- Sistema siempre llega a RUN

---

## Resumen de Aserciones

| Test | Aserciones | Descripción |
|------|-----------|-------------|
| 1 | 6 | Boot sequence básico |
| 2 | 4 | Requisito start + brake |
| 3 | 5 | R2D delay 2000ms |
| 4 | 1 | READY → RUN |
| 5 | 6 | Torque output |
| 6 | 7 | Secuencia completa |
| 7 | 3 | Precharge recovery |
| 8 | 1000 | Validación estado máquina |
| 9 | 20 | Ciclos rápidos |
| **TOTAL** | **1052** | **Cobertura completa** |

---

## Uso en main.c

```c
#include "test_startup_sequence.h"

int main(void) {
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  /* ... otros inicios ... */

  /* USER CODE BEGIN SysInit */
  Diag_Log("\n=== ECU08 NSIL Boot ===\n");
  
  // Ejecutar test de arranque
  TestStats_Startup_t startup_stats = Test_StartupSequenceRunAll();
  
  // Opcionalmente: ejecutar otros tests
  // TestStats_t advanced_stats = Test_AdvancedRunAll();
  
  /* USER CODE END SysInit */

  MX_FREERTOS_Init();
  osKernelStart();

  return 0;
}
```

---

## Salida Esperada (UART 115200 baud)

```
╔══════════════════════════════════════════════════════════╗
║      STARTUP SEQUENCE TEST SUITE - ECU08 NSIL            ║
╚══════════════════════════════════════════════════════════╝

┌─ TEST 1: Basic Boot Sequence ─────────────────────────────────────┐
├─ BOOT → PRECHARGE_ACK → START_BRAKE
[✓] Initial state is BOOT
[✓] No precharge → WAIT_PRECHARGE_ACK
[✓] Stays in WAIT_PRECHARGE_ACK
[✓] Precharge OK → WAIT_START_BRAKE

┌─ TEST 2: Start Button & Brake Requirement ─────────────────────┐
├─ Must have both start button AND brake pressed
[✓] Ready to check start conditions
[✓] Start without brake: rejected
[✓] Brake without start: rejected
[✓] Start + Brake → R2D_DELAY

┌─ TEST 3: R2D Delay (2 seconds) ─────────────────────────────────┐
├─ Ready-To-Drive delay must be exactly 2000ms
[✓] Entered R2D_DELAY state
[✓] R2D timer reset to 0ms
[✓] Still in R2D_DELAY at 100ms
[✓] R2D timer at 100ms
[✓] Still in R2D_DELAY at 1500ms
[✓] R2D delay complete → READY at 2000ms+

┌─ TEST 4: READY → RUN Transition ─────────────────────────────────┐
├─ Transition to RUN state for torque output
[✓] READY → RUN on next step

┌─ TEST 5: Torque Output in RUN State ─────────────────────────────┐
├─ Validate torque calculation and CAN output
[✓] Zero throttle → 0% torque
[✓] CAN frame queued
[✓] 50% throttle → ~50% torque
[✓] Multiple CAN frames queued
[✓] Full throttle → 100% torque
[✓] Continuous CAN output

┌─ TEST 6: Complete Startup Sequence ─────────────────────────────┐
├─ Full path: BOOT → PRECHARGE_ACK → START_BRAKE → R2D → READY → RUN
[✓] Step 0: BOOT
[✓] Phase 1: Waiting precharge
[✓] Phase 2: Precharge OK
[✓] Phase 3: Start + Brake detected
[✓] Phase 4: R2D delay complete
[✓] Phase 5: RUN state reached
[✓] Phase 6: Torque output active
[✓] Phase 6: CAN frames transmitted

┌─ TEST 7: Precharge Failure & Recovery ─────────────────────────┐
├─ System handles precharge failure gracefully
[✓] Waiting for precharge
[✓] Precharge recovered
[✓] Normal startup proceeds after recovery

┌─ TEST 8: State Machine Validation ─────────────────────────────┐
├─ All state transitions remain within valid range
[✓] Valid state (x1000)

┌─ TEST 9: Rapid Start-Stop Cycling ─────────────────────────────┐
├─ System handles rapid on/off cycles
[✓] Cycle complete (x20)

┌──────────────────────────────────────────────────────────────────┐
│                  STARTUP TEST SUMMARY                            │
├──────────────────────────────────────────────────────────────────┤
│ Total Assertions:    1052                                        │
│ Passed:              1052 ✓                                      │
│ Failed:              0 ✗                                         │
│ Pass Rate:           100.0%                                      │
└──────────────────────────────────────────────────────────────────┘
```

---

## Máquina de Estados Simulada

```
                    ┌─────────────────┐
                    │      BOOT       │
                    └────────┬────────┘
                             │
                    ┌────────▼────────────────┐
         NO         │ ok_precarga?            │
    ┌───────────────│                         │
    │               └────────┬────────────────┘
    │                        │ YES
    │         ┌──────────────▼──────────────┐
    │         │ WAIT_PRECHARGE_ACK          │
    │         └──────────┬───────────────────┘
    │                    │
    │      ┌─────────────▼──────────────┐
    │      │ WAIT_START_BRAKE           │
    │      │ (need: boton_arranque &&   │
    │      │  s_freno > 3000)           │
    │      └──────────┬───────────────────┘
    │                 │
    │      ┌──────────▼───────────────┐
    │      │ R2D_DELAY (2000ms)       │
    │      └──────────┬───────────────┘
    │                 │
    │      ┌──────────▼───────────────┐
    │      │ READY                    │
    │      └──────────┬───────────────┘
    │                 │
    └──────────────┬──▼───────────────┐
                   │ RUN              │
                   │ (send torque)    │
                   └──────────────────┘
```

---

## Integración Paso a Paso

### Paso 1: Incluir header
```c
/* USER CODE BEGIN Includes */
#include "test_startup_sequence.h"
/* USER CODE END Includes */
```

### Paso 2: Llamar en main()
```c
/* USER CODE BEGIN SysInit */
TestStats_Startup_t results = Test_StartupSequenceRunAll();
/* USER CODE END SysInit */
```

### Paso 3: Compilar
```
Ctrl+B en STM32CubeIDE
```

### Paso 4: Ver salida
Terminal serial 115200 baud

---

## Requisitos

- ✓ `diag.h/diag.c` - Para `Diag_Log()` (UART output)
- ✓ `app_state.h` - Incluido en simulador
- ✓ `control.h` - Para estructuras CAN

---

## Troubleshooting

| Problema | Solución |
|----------|----------|
| Compilation Error: undefined reference | Verificar que diag.c esté en build |
| No output en UART | 115200 baud, reiniciar conexión |
| Tests fallan aleatoriamente | Revisar simulador - bug en lógica de estado |
| Timeout en test 8 | 1000 aserciones pueden ser lentas |

---

## Notas Técnicas

- **Simulador embebido**: No necesita hardware real, solo firmware compilado
- **Máquina de estados**: Copia fiel de la en `control.c`
- **Timers**: Simulados en múltiplos de 10ms
- **No bloqueante**: <1 segundo para ejecutar todos los tests
- **Compatible FreeRTOS**: Ejecuta ANTES de `osKernelStart()`

---

**Fecha**: 25 de enero de 2026  
**Tests**: 9 suites completas  
**Aserciones**: 1052  
**Cobertura**: Secuencia completa BOOT→RUN
