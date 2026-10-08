# 1.2.0 Stable Build

## Changes

- Updated the Modern UI, theme selection, brightness control, and focus states.
  The Classic UI remains available as a build option.
- Separated fixed-voltage and PPS pages. Fixed-voltage options follow the
  charger's advertised PDOs; PPS requires an advertised APDO.
- Enabled Chinese/English switching without rebooting and fixed intermittent
  language-switch lockups and page-transition display corruption.
- Improved cold-start and PD negotiation when powered only through the input.
- Fixed CMake build dependencies and application stack layout for the existing
  Bootloader. The Bootloader image is unchanged.
- Synchronized Keil source entries and enabled the Modern UI. Added an
  ARMCC 5 static-assert compatibility header without changing application code.
- Kept upstream 1.1.7 CMD commands and calibration wire format, plus the
  `update\r\n` OTA entry.
- Added USB receive backpressure for fragmented CMD frames and a dedicated
  transmit buffer. CMD replies take priority over measurement packets.
- Reduced CMD RX/TX and UI measurement queues to two entries and bounded CMD
  receive batches.
- Matched the upstream EEPROM settings/calibration layout at `0x40`, retained
  the update flag at `0x20`, and added write/readback checks and CMD error replies.
- Added startup and fault diagnostics. Detailed event logs are held in RAM;
  reset recovery records use backup registers and EEPROM. Full logs do not
  survive power loss.
- Disabled the unused LVGL Chart module, saving 5,376 bytes of Flash.

This version number does not indicate official upstream release status.

## Release Image

Filename: `firmware/PowerPico_Firmware_v1.2.0.bin`

SHA256: `E229043E0F34DA95F139D456041998CC05327760C1E9450CA0EDE06EFCD97303`

Image size: 444572 bytes. Application flash budget: 458752 bytes.
Remaining application flash: 14180 bytes.
Linker RAM reservation: 114264 of 131072 bytes.
The RAM figure includes reserved heap pools; it is not a runtime heap/stack
free-space measurement.

## Validation

- The release image was rebuilt from the cleaned source tree on October 7, 2026.
- The previously hardware-tested firmware used the same application baseline and
  was uploaded successfully through the existing Bootloader/OTA path.
- Fragmented CMD regression: 80/80 cases passed.
- CMD plus continuous ADC duplex regression: 200/200 replies passed, with no
  missing or unexpected frames.
- Keil Modern build passed with ARM Compiler 5.06 update 5 (build 528).
  Its application vector table was checked; hardware flashing has not been
  tested for the Keil image.
