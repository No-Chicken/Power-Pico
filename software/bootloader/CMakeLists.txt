cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "MinSizeRel")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME BootLoader)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# 强制 CMake 和 GCC/Clang 永远输出彩色日志
set(CMAKE_COLOR_DIAGNOSTICS ON)
add_compile_options(-fdiagnostics-color=always)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)


# Create output directory for hex and bin files
set(OUTPUT_PATH ${CMAKE_BINARY_DIR}/output)
file(MAKE_DIRECTORY ${OUTPUT_PATH})

# Add post-build step to generate hex and bin files
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${OUTPUT_PATH}/${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${OUTPUT_PATH}/${CMAKE_PROJECT_NAME}.bin
    COMMENT "Generating hex and bin files")

file(GLOB_RECURSE C_SOURCES "BSP/**/*.c"  "SYSTEM/*.c" "Ymodem/*.c")
set(H_DIRS "BSP/EEPROM" "BSP/KEY" "BSP/LCD" "SYSTEM" "USB_DEVICE/App" "Ymodem")

set(AS_SOURCES
    startup_stm32f411xe.s
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${C_SOURCES}
    ${AS_SOURCES}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${H_DIRS}
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    USE_HAL_DRIVER
    STM32F411xE
)
