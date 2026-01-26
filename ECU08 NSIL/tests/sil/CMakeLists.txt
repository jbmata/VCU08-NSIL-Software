cmake_minimum_required(VERSION 3.15)

# SIL (Software-In-The-Loop) environment
# Simulates complete application logic without requiring FreeRTOS compilation

# Application sources (only logic, no RTOS dependencies)
set(APP_SOURCES
    ../../Core/Src/app_state.c
    ../../Core/Src/can.c
    ../../Core/Src/control.c
    ../../Core/Src/telemetry.c
)

# SIL simulation sources
set(SIL_SOURCES
    sil_main.c
    sil_hal_mocks.c
    sil_can_simulator.c
    sil_boot_sequence.c
    sil_results.c
    integration/test_boot_sequence.c
    integration/test_full_cycle.c
)

# Create executable
add_executable(ecu08_sil
    ${APP_SOURCES}
    ${SIL_SOURCES}
)

# Include directories
target_include_directories(ecu08_sil PRIVATE
    ../../Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

# Compilar con warnings
target_compile_options(ecu08_sil PRIVATE
    -Wall -Wextra -pedantic
    -DSIL_BUILD=1
)

# Add test target
enable_testing()
add_test(
    NAME SILBootSequence
    COMMAND ecu08_sil --test-boot
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
    NAME SILFullCycle
    COMMAND ecu08_sil --test-full-cycle
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

