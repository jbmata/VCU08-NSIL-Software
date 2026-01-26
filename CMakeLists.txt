cmake_minimum_required(VERSION 3.15)
project(ECU08_NSIL_Tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Opción: compilar tests host
option(BUILD_HOST_TESTS "Build host unit tests" ON)

if(BUILD_HOST_TESTS)
    # Usar GCC nativo para host
    if(NOT CMAKE_C_COMPILER_WORKS)
        find_program(GCC_COMPILER gcc)
        if(GCC_COMPILER)
            set(CMAKE_C_COMPILER ${GCC_COMPILER})
        endif()
    endif()
    
    message(STATUS "Building HOST Unit Tests with ${CMAKE_C_COMPILER}")
    add_subdirectory(test_host)
else()
    message(STATUS "Host tests DISABLED. To enable: -DBUILD_HOST_TESTS=ON")
endif()

# Información de compilación
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")
