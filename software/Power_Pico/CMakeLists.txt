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

# =========================================================
# 创建一个虚拟的全局配置包裹 (INTERFACE 表示它不编译任何代码，只提供属性)
add_library(sys_config INTERFACE)

# 把系统需要的所有核心路径全塞进包裹里
target_include_directories(sys_config INTERFACE
    ${CMAKE_SOURCE_DIR}/Core/Inc
    ${CMAKE_SOURCE_DIR}/USB_DEVICE/App
    ${CMAKE_SOURCE_DIR}/USB_DEVICE/Target
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/include
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
    ${CMAKE_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
)

# 把核心的宏定义也塞进包裹里
target_compile_definitions(sys_config INTERFACE
    USE_HAL_DRIVER
    STM32F411xE
)

# 挂载子目录
add_subdirectory(Middlewares/LVGL)
add_subdirectory(BSP)
add_subdirectory(User)

# =========================================================

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    # Add user defined libraries
    lvgl
    bsp
    user_app
)
