# Quick Start: Ejecución de Tests

## ⚡ Inicio Rápido

### Windows (PowerShell)

```powershell
# Tests unitarios
.\run_tests.ps1 unit

# Tests SIL (FreeRTOS simulado)
.\run_tests.ps1 sil

# Ambos
.\run_tests.ps1 all
```

### Linux/macOS (Bash)

```bash
# Tests unitarios
./run_tests.sh unit

# Tests SIL
./run_tests.sh sil

# Ambos
./run_tests.sh all
```

## 📋 Tests Disponibles

### 1. Tests Unitarios (`tests/unit/`)
- **Ejecución**: ~100ms
- **Cantidad**: 34 tests
- **Framework**: Unity
- **Módulos**:
  - CAN parsing (13 tests)
  - Control logic (12 tests)
  - Telemetry (9 tests)

```bash
# Ejecutar solo unitarios
cmake -B build -DBUILD_UNIT_TESTS=ON -DBUILD_SIL_TESTS=OFF
cmake --build build
cd build && ctest
```

### 2. Entorno SIL (`tests/sil/`)
- **Ejecución**: ~10 segundos
- **Framework**: FreeRTOS simulado + custom tests
- **Simula**: Boot sequence completa
- **Incluye**: inyector de CAN, HAL mocks

```bash
# Boot sequence test
./build/tests/sil/ecu08_sil --test-boot

# Full cycle test
./build/tests/sil/ecu08_sil --test-full-cycle
```

## 🔧 Configuración Manual

### Compilar solo unitarios
```bash
cmake -B build -DBUILD_UNIT_TESTS=ON
cmake --build build
ctest --test-dir build
```

### Compilar solo SIL
```bash
cmake -B build -DBUILD_SIL_TESTS=ON
cmake --build build
ctest --test-dir build
```

### Compilar ambos
```bash
cmake -B build
cmake --build build
ctest --test-dir build
```

## 📊 Verificar Cobertura

### Unit tests con coverage
```bash
cd build/tests/unit
# Generar reporte LCOV
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
# Ver en navegador
open coverage_html/index.html  # macOS
xdg-open coverage_html/index.html  # Linux
start coverage_html/index.html  # Windows
```

## 🐛 Debugging

### GDB con tests unitarios
```bash
gdb ./build/tests/unit/ecu08_unit_tests
(gdb) run
(gdb) bt  # backtrace si hay crash
```

### GDB con SIL
```bash
gdb ./build/tests/sil/ecu08_sil
(gdb) run --test-boot
```

## 📁 Estructura

```
tests/
├── unit/                          # Tests unitarios (rápidos)
│   ├── CMakeLists.txt
│   ├── mocks.h / mocks.c         # Mocks de RTOS y HAL
│   ├── test_can_parsing.c
│   ├── test_control_logic.c
│   ├── test_telemetry.c
│   └── unity_runner.c
│
└── sil/                           # Entorno SIL (realista)
    ├── CMakeLists.txt
    ├── sil_main.c                # Entry point
    ├── sil_hal_mocks.h / .c      # Mocks del HAL
    ├── sil_can_simulator.h / .c  # Inyector CAN
    ├── sil_boot_sequence.h / .c  # Tracking de boot
    └── integration/
        ├── test_boot_sequence.c
        └── test_full_cycle.c
```

## ✅ Requisitos

- **CMake** ≥ 3.15
- **GCC** (nativo para host tests)
- **Opcional**: LCOV/GENHTML (para coverage)

### Instalar (Ubuntu/Debian)
```bash
sudo apt-get install cmake gcc lcov
```

### Instalar (macOS)
```bash
brew install cmake gcc lcov
```

### Instalar (Windows)
- CMake: https://cmake.org/download/
- MinGW GCC: https://www.mingw-w64.org/ o MSYS2
- LCOV: https://github.com/linux-test-project/lcov (opcional)

## 🚨 Troubleshooting

### "cmake: command not found"
→ Instalar CMake o agregar a PATH

### "gcc: command not found"
→ Instalar GCC (MinGW en Windows)

### Tests no se ejecutan
→ Asegurarse que se compilaron: `cmake --build build`

### SIL lento
→ Normal, está ejecutando FreeRTOS simulado (~10s)

## 📖 Más Información

Ver [TESTING_STRUCTURE.md](TESTING_STRUCTURE.md) para documentación completa.
