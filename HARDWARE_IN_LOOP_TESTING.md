# Hardware in-Loop Testing - ECU08 NSIL + Raspberry Pi

## 🏗️ Arquitectura de Testing

```
┌─────────────────────────────┐
│   Raspberry Pi 4            │
│   (Test Harness Python)     │
│   ├─ test_rpi_harness.py   │
│   ├─ 10 Test Cases          │
│   └─ Serial Monitor         │
└────────────┬────────────────┘
             │ UART @ 115200
    ┌────────▼──────────┐
    │  USB/Serial Cable │
    └────────┬──────────┘
             │
┌────────────▼───────────────────┐
│   STM32H733ZGT6 (ECU08)         │
│   ├─ Firmware Running           │
│   ├─ test_rpi_commands.c        │
│   ├─ Command Parser             │
│   └─ Response Generator         │
└────────────────────────────────┘
```

---

## 📋 10 Hardware In-Loop Tests

| # | Test | Descripción | Protocolo |
|---|------|-------------|-----------|
| 1 | UART Communication | Verificar comunicación serial | TEST_UART_START |
| 2 | ADC Sensor Readings | Leer valores de sensores | READ_SENSORS |
| 3 | Torque Calculation | Calcular torque desde throttle | SET_THROTTLE XX |
| 4 | State Machine | Verificar estado actual | READ_STATE |
| 5 | CAN Communication | Status de buses CAN | READ_CAN_STATUS |
| 6 | EV2.3 Safety | Freno + throttle inhibición | TEST_EV23_BRAKE_THROTTLE |
| 7 | Startup Sequence | BOOT→PRECHARGE→START→R2D→RUN | STARTUP_PHASE_X |
| 8 | Performance Timing | Latencia de respuesta | PING |
| 9 | Memory Status | Estado de memoria/recursos | READ_MEMORY_STATUS |
| 10 | Firmware Version | Información de firmware | READ_FIRMWARE_VERSION |

---

## 🚀 Instalación en Raspberry Pi

### Requisitos

```bash
# Actualizar sistema
sudo apt-get update
sudo apt-get upgrade -y

# Instalar dependencias
sudo apt-get install -y python3 python3-pip
pip3 install pyserial

# Crear directorio de tests
mkdir -p ~/ecu08_tests
cd ~/ecu08_tests
```

### Copiar Script

```bash
# Copiar rpi_test_harness.py a Raspberry Pi
scp rpi_test_harness.py pi@raspberrypi.local:~/ecu08_tests/

# O copiar manualmente via SCP/FTP
```

### Permisos

```bash
chmod +x ~/ecu08_tests/rpi_test_harness.py
```

---

## 🔌 Conexión Física

### UART Connection

```
STM32H733 (UART1)          Raspberry Pi
├─ PA9  (TX) ────────────→ GPIO 15 (RX) 
├─ PA10 (RX) ←──────────── GPIO 14 (TX)
└─ GND  ────────────────── GND
```

**O via adaptador USB-Serial:**

```
STM32H733         USB Adapter        Raspberry Pi
├─ PA9  (TX) → TX ──┬─ USB Port
├─ PA10 (RX) → RX ──┤ (/dev/ttyUSB0)
└─ GND  ────→ GND ──┴─ GND
```

### Configuración Puerto Serial

```bash
# Listar puertos disponibles
ls /dev/tty*

# Típicamente:
# - /dev/ttyUSB0 (si es adaptador USB)
# - /dev/ttyAMA0 (GPIO 14/15 en RPi)
```

---

## 🧪 Ejecutar Tests

### Opción 1: Con Adaptador USB

```bash
cd ~/ecu08_tests
python3 rpi_test_harness.py --port /dev/ttyUSB0 --baud 115200
```

### Opción 2: GPIO Serial (RPi nativo)

```bash
# Habilitar UART en GPIO
sudo raspi-config
# Serial → Habilitar Puerto Serial

# Ejecutar con puerto GPIO
python3 rpi_test_harness.py --port /dev/ttyAMA0 --baud 115200
```

### Opción 3: Con Parámetros Personalizados

```bash
python3 rpi_test_harness.py \
  --port /dev/ttyUSB0 \
  --baud 115200 \
  --timeout 5
```

---

## 📊 Salida Esperada

```
✓ Conectado a /dev/ttyUSB0 @ 115200 baud

┌─ MENSAJES DE BOOT ──────────────────────────────────┐
  ╔════════════════════════════════════════════════════════════╗
  ║   VERIFICACIÓN COMPLETA DEL SISTEMA - ECU08 NSIL          ║
  ║   Validación: Hardware → Lógica → Comunicación            ║
  ╚════════════════════════════════════════════════════════════╝
  (Boot messages...)
└─────────────────────────────────────────────────────┘

┌─ TEST 1: COMUNICACIÓN UART ────────────────────────────┐
  ✓ Comunicación UART OK
  Respuesta: [TEST_UART] OK - STM32H733 @ 480MHz
└─────────────────────────────────────────────────────┘

┌─ TEST 2: LECTURA DE SENSORES ADC ──────────────────────┐
  Sensor: S1_ACCEL: 2050 ADC
  Sensor: S2_ACCEL: 1915 ADC
  Sensor: BRAKE: 0 ADC
  Sensor: START_BTN: 0
  Sensor: PRECHARGE: 0
  ✓ Sensores leyendo correctamente
└─────────────────────────────────────────────────────┘

┌─ TEST 3: CÁLCULO DE TORQUE ────────────────────────────┐
  ✓ Torque calculado: [TORQUE] 50% (from 50% throttle)
└─────────────────────────────────────────────────────┘

┌─ TEST 4: MÁQUINA DE ESTADOS ───────────────────────────┐
  ✓ Estado: [STATE_INFO]
    TORQUE: 50%
    EV2.3_FLAG: 0
    TEMP_INV: 25 C
    VOLTAGE: 380 mV
└─────────────────────────────────────────────────────┘

┌─ TEST 5: COMUNICACIÓN CAN ─────────────────────────────┐
  ✓ CAN Status: [CAN_STATUS]
    BUS1: OK
    BUS2: OK
    BUS3: OK
└─────────────────────────────────────────────────────┘

┌─ TEST 6: SEGURIDAD EV2.3 ──────────────────────────────┐
  ✓ EV2.3 Active: [EV2.3_TEST]
    Brake + High Throttle
    EV2.3_FLAG: 1
    TORQUE_INHIBITED: 1
└─────────────────────────────────────────────────────┘

┌─ TEST 7: SECUENCIA DE ARRANQUE ────────────────────────┐
  Fase 1: BOOT
    [STARTUP] OK
  Fase 2: PRECHARGE
    [STARTUP] OK
  Fase 3: START+BRAKE
    [STARTUP] OK
  ✓ Secuencia completa OK
└─────────────────────────────────────────────────────┘

┌─ TEST 8: RENDIMIENTO Y TIMING ─────────────────────────┐
  ✓ Latencia: 5.2ms (< 100ms)
└─────────────────────────────────────────────────────┘

┌─ TEST 9: ESTADO DE RECURSOS ───────────────────────────┐
  ✓ Recursos: [MEMORY_STATUS]
    HEAP: OK
    STACK: OK
    DATA_SIZE: 256 bytes
    FLASH_USAGE: ~65%
└─────────────────────────────────────────────────────┘

┌─ TEST 10: INFORMACIÓN FIRMWARE ────────────────────────┐
  ✓ Firmware: [FIRMWARE]
    VERSION: 1.0
    DATE: 2026-01-26
    MCU: STM32H733ZGT6
    BUILD: Debug
└─────────────────────────────────────────────────────┘

╔════════════════════════════════════════════════════════════╗
║                   RESUMEN DE TESTS                         ║
╠════════════════════════════════════════════════════════════╣
║ Total Tests:           10                                 ║
║ Pasados:               10 ✓                               ║
║ Fallidos:              0 ✗                                ║
║ Tasa de Éxito:         100.0%                             ║
╠════════════════════════════════════════════════════════════╣
║ RESULTADO: ✅ TODOS LOS TESTS PASARON                     ║
╚════════════════════════════════════════════════════════════╝

✓ Desconectado
```

---

## 🔧 Integración STM32 - Agregar Handler UART

Para que el STM32 responda a comandos de RPi, agregue en `main.c`:

```c
#include "test_rpi_commands.h"

/* En la tarea de control o UART ISR */
void USART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    char received_line[128];
    /* Read received data */
    /* ... */
    
    /* Process command from RPi */
    RPI_ProcessIncomingCommand(received_line);
  }
}
```

O crear una tarea dedicada:

```c
void RPICommandTask(void *argument)
{
  char line[128];
  
  while (1)
  {
    /* Read from UART */
    if (HAL_UART_Receive(&huart1, (uint8_t*)line, 128, 100) == HAL_OK)
    {
      RPI_ProcessIncomingCommand(line);
    }
    osDelay(10);
  }
}
```

---

## 📋 Protocolo de Comunicación

### Formato General

```
RPi → STM32:  COMMAND [PARAM1] [PARAM2]
STM32 → RPi:  [TAG] DATA
```

### Ejemplos

```
# Request
TEST_UART_START

# Response
[TEST_UART] OK - STM32H733 @ 480MHz

---

# Request
SET_THROTTLE 50

# Response
[TORQUE] 50% (from 50% throttle)

---

# Request
PING

# Response
[PONG] STM32H733 ECU08 alive
```

---

## ✅ Checklist de Validación

- [ ] RPi conectada a STM32 por UART
- [ ] Puerto serial identificado (`/dev/ttyUSB0` o `/dev/ttyAMA0`)
- [ ] Baud rate 115200 configurado
- [ ] `rpi_test_harness.py` copiado a RPi
- [ ] `test_rpi_commands.c/h` compilado en firmware STM32
- [ ] UART handler implementado en STM32
- [ ] Tests ejecutados: `python3 rpi_test_harness.py`
- [ ] 10/10 tests pasando
- [ ] Latencia < 100ms
- [ ] Recursos OK

---

## 🛠️ Troubleshooting

| Problema | Causa | Solución |
|----------|-------|----------|
| Connection refused | Puerto incorrecto | `ls /dev/tty*` para listar puertos |
| Baud rate error | Mismatch STM32↔RPi | Verificar ambos @ 115200 |
| No response | UART no inicializado | Verificar MX_UART_Init() |
| Latencia alta | Buffer lleno | Aumentar tamaño buffer UART |
| Memory error | Stack overflow | Reducir tamaño líneas comando |

---

## 📈 Escalabilidad

Para agregar más tests:

1. Agregar enum en `test_rpi_commands.h`:
   ```c
   typedef enum {
     // ... existing commands ...
     CMD_MY_NEW_TEST,
   } rpi_command_t;
   ```

2. Actualizar parser en `test_rpi_commands.c`:
   ```c
   if (strncmp(cmd_str, "MY_COMMAND", 10) == 0) 
     return CMD_MY_NEW_TEST;
   ```

3. Implementar handler:
   ```c
   case CMD_MY_NEW_TEST:
     // Your test logic
     break;
   ```

4. Agregar test en `rpi_test_harness.py`:
   ```python
   def test_my_new_feature(self):
       self.write_command("MY_COMMAND")
       # Read response
   ```

---

## 📚 Archivos Necesarios

| Archivo | Ubicación | Descripción |
|---------|-----------|-------------|
| `rpi_test_harness.py` | RPi home dir | Test harness Python |
| `test_rpi_commands.h` | STM32 Core/Inc | Header de comandos |
| `test_rpi_commands.c` | STM32 Core/Src | Implementación de comandos |

---

## 🎯 Resultado Esperado

**10/10 tests PASS** ✅

- ✓ UART working
- ✓ Sensors reading
- ✓ Torque calculating
- ✓ State machine running
- ✓ CAN buses active
- ✓ EV2.3 safety working
- ✓ Startup sequence valid
- ✓ Response time < 100ms
- ✓ Memory OK
- ✓ Firmware running

---

**Fecha:** 26 de enero de 2026  
**Hardware:** STM32H733 + Raspberry Pi 4  
**Tests:** 10 suites in-loop  
**Status:** ✅ Ready for validation
