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
set(CMAKE_PROJECT_NAME power_pico)

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
option(POWER_PICO_UI_MODERN "Build the modern instrument UI instead of Classic" OFF)
option(POWER_PICO_LTO "Enable link-time optimization for a smaller firmware image" OFF)
if(POWER_PICO_LTO)
    add_compile_options(-flto)
    add_link_options(-flto)
endif()
option(POWER_PICO_RESTORE_EEPROM_BASELINE "Build one-shot EEPROM rollback helper" OFF)
if(POWER_PICO_RESTORE_EEPROM_BASELINE)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE POWER_PICO_RESTORE_EEPROM_BASELINE=1)
endif()
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    POWER_PICO_UI_MODERN=$<BOOL:${POWER_PICO_UI_MODERN}>)
# Both UI themes format measurement values with floating-point snprintf.
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -Wl,-u,_printf_float)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)
if(POWER_PICO_LTO AND TARGET FreeRTOS)
    # FreeRTOS port references vTaskSwitchContext from assembly; keep that
    # object library non-LTO while allowing application/LVGL dead-code folding.
    target_compile_options(FreeRTOS PRIVATE -fno-lto)
endif()

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Collect all source files using file globbing
file(GLOB_RECURSE LVGL_SOURCES
    "Middlewares/LVGL/src/*.c"
    "Middlewares/LVGL/porting/*.c"
)
# Disabled NuttX sources require headers absent from this STM32 distribution.
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/src/drivers/nuttx/.*")
file(GLOB_RECURSE USER_SOURCES "User/*.c")
file(GLOB_RECURSE BSP_SOURCES "BSP/**/*.c")


# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${LVGL_SOURCES}
    ${USER_SOURCES}
    ${BSP_SOURCES}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Middlewares/LVGL
    Middlewares/LVGL/src
    Middlewares/LVGL/porting
    BSP
    BSP/BEEP
    BSP/EEPROM
    BSP/GATE
    BSP/KEY
    BSP/LCD
    BSP/PD
    User
    User/Tasks/Inc
    User/GUI
    User/GUI/common
    User/GUI/screens
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# Create output directory for .hex and .bin files
set(OUTPUT_PATH ${CMAKE_BINARY_DIR}/output)
file(MAKE_DIRECTORY ${OUTPUT_PATH})

# Add custom target to generate .hex and .bin files
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${OUTPUT_PATH}/${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${OUTPUT_PATH}/${CMAKE_PROJECT_NAME}.bin
    COMMENT "Generating .hex and .bin files"
)
