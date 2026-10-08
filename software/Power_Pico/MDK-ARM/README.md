# Keil Build

Open `power_pico.uvprojx` in Keil uVision 5 and build the `power_pico` target.
The project uses ARM Compiler 5, the RVDS FreeRTOS port, and the Modern UI.
Install the STM32F4xx device pack selected by the project.

Output: `power_pico/power_pico.bin`. The application starts at `0x08010000`;
flash it through the existing Bootloader and client.

`armcc_compat.h` provides C11-style static assertions for ARMCC 5 through the
project's preinclude option. Application source is shared with the CMake build.

Verified with ARM Compiler 5.06 update 5 (build 528). The Keil image has been
compiled and its vector table checked, but has not been flashed to hardware.
