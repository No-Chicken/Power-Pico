<p align="right">
  <a href="./README.md">中文</a> | <b>English</b>
</p>

<div align="center">
  <h1 align="center">Power-Pico</h1>
  <p align="center">A Product-Grade Portable USB Ammeter and Low-Power Measurement Analyzer</p>

  <img src="https://img.shields.io/badge/Version-1.0.5-blue">
  <img src="https://img.shields.io/badge/License-Apache2.0-green">
  <img src="https://img.shields.io/badge/MCU-STM32F411-lightblue">
  <img src="https://img.shields.io/badge/UI-LVGL_v9.2-red">
  <img src="https://img.shields.io/badge/OS-FreeRTOS-green">
  <br>
  <img src="https://img.shields.io/github/stars/kingham/Power-Pico?style=flat-square">
</div>

---

## 📖 Project Introduction

**Power-Pico** is more than just a USB ammeter. It is a **high-precision, low-power analysis tool** designed specifically for embedded developers. To overcome the physical limitations of traditional ammeters in the μA-level measurement, Power-Pico adopts an innovative **multi-range auto-switching sampling** design, achieving full-range coverage from $1\mu A$ to $5A$.

<div style="margin-top:20px;text-align:center">
    <img src="./images/main2316.png" style="width:75%;border-radius:16px;box-shadow:0 8px 24px rgba(0,0,0,0.1);display:inline-block" alt="Display" />
</div>

### ✨ Core Features
- **μA-Level High Precision**: Utilizes the INA190 zero-drift amplifier with three-resistor segmented sampling.
- **Protocol Trigger**: Supports PD2.0 PPS protocol triggering, with up to 20V/5A output.
- **High-Performance UI**: Based on STM32F411 + LVGL v8.3, providing a smooth real-time waveform display.
- **Cross-Platform PC Client**: Comes with the Power-Pico Client developed with PySide6, supporting data export and real-time power analysis.
- **3D Printed Enclosure**: Provides a complete design for a 3D printed enclosure and acrylic panel.

---

## 🛠️ Hardware Parameter Analysis

| Module | Model | Advantage |
| :--- | :--- | :--- |
| **MCU** | STM32F411CEU6 | 100MHz high frequency, supports DSP instructions to accelerate waveform processing |
| **Current Sensing** | INA190 | 0.5nA input bias current, the core guarantee for μA-level measurement |
| **ADC** | 12-bit (STM32 Internal) | Achieves high-speed stable sampling above 10kHz with DMA |
| **Screen** | 1.54" TFT (ST7789) | 240x240 resolution, high-contrast display |

---

## 🚀 Quick Replication

### 1. Hardware
Solder the hardware yourself, or purchase a pre-soldered and assembled kit from Taobao.
LCSC Open Source Hub page: https://oshwhub.com/no_chicken/powerpico

### 2. Firmware Compilation and Flashing
The project is developed based on Keil MDK:
1. Clone the repository: `git clone https://github.com/kingham/Power-Pico.git`
2. Flash the BootLoader via the SWD port.
3. Compile PowerPico yourself. The generated `.bin` file can then be flashed using the PowerPico Client.

### 3. Assembly

The physical assembly is shown in the figure below:


The exploded 3D view is as follows:

<div style="display:flex;gap:15px;justify-content:center;align-items:center;margin-bottom:20px">

<div style="flex:1;display:flex;align-items:center;justify-content:center;border-radius:12px;overflow:hidden;">
    <img src="./images/boom.png" style="max-width:100%;max-height:100%;width:auto;height:auto;display:block" alt="3D Exploded View Left" />
</div>

<div style="flex:1;display:flex;align-items:center;justify-content:center;border-radius:12px;overflow:hidden;">
    <img src="./images/boom.gif" style="max-width:100%;max-height:100%;width:auto;height:auto;display:block" alt="3D Exploded View Right" />
</div>
</div>

### 4. Running the PC Client
Install and use the PowerPico Client. Currently, it only supports Windows.

## 🛠️ User Guide

1. Hardware Operation

    The button operations and various interface displays of Power-Pico are shown below. The top button is the **Page Switch/Boot** button. Press and hold the top button before powering on to enter Boot mode for a forced firmware upgrade.

    The Power-Pico hardware currently supports 2 languages: **Simplified Chinese/English**.

    <div style="display:flex;gap:15px;justify-content:center;align-items:center;margin-bottom:20px">

    <div style="flex:1;display:flex;align-items:center;justify-content:center;border-radius:12px;overflow:hidden;">
        <img src="./images/key_func.png" style="max-width:100%;max-height:100%;width:auto;height:auto;display:block" alt="Button Functions" />
    </div>

    <div style="flex:1;display:flex;align-items:center;justify-content:center;border-radius:12px;overflow:hidden;">
        <img src="./images/startup_page.png" style="max-width:100%;max-height:100%;width:auto;height:auto;display:block" alt="Startup Page" />
    </div>

    <div style="flex:1;display:flex;align-items:center;justify-content:center;border-radius:12px;overflow:hidden;">
        <img src="./images/main_page.png" style="max-width:100%;max-height:100%;width:auto;height:auto;display:block" alt="Main Page" />
    </div>

    </div>

    <div style="display:flex;gap:15px;justify-content:center;align-items:center;margin-bottom:20px">

    <div style="flex:1;display:flex;align-items:center;justify-content:center;border-radius:12px;overflow:hidden;">
        <img src="./images/set_page.png" style="max-width:100%;max-height:100%;width:auto;height:auto;display:block" alt="Settings Page" />
    </div>

    <div style="flex:1;display:flex;align-items:center;justify-content:center;border-radius:12px;overflow:hidden;">
        <img src="./images/pps_page1.png" style="max-width:100%;max-height:100%;width:auto;height:auto;display:block" alt="PPS Page 1" />
    </div>

    <div style="flex:1;display:flex;align-items:center;justify-content:center;border-radius:12px;overflow:hidden;">
        <img src="./images/pps_page2.png" style="max-width:100%;max-height:100%;width:auto;height:auto;display:block" alt="PPS Page 2" />
    </div>

    </div>

2. Client Operation

    The PowerPico Client for PC can be used for waveform viewing, recording, exporting, and upgrading the firmware of the hardware. The client currently supports 4 languages: **Simplified Chinese/Traditional Chinese/English/Japanese**.

    For detailed instructions on using the PowerPico Client, please refer to: [Client Operation Manual](https://no-chicken.com/content/Power-Pico/UserManual/client_operation.html)

    <div style="display:flex;gap:15px;justify-content:center;align-items:center;margin-bottom:20px">

    <div style="flex:1;display:flex;align-items:center;justify-content:center;border-radius:12px;overflow:hidden;">
        <img src="./images/wave_client1.png" style="max-width:100%;max-height:100%;width:auto;height:auto;display:block" alt="Waveform Interface" />
    </div>

    <div style="flex:1;display:flex;align-items:center;justify-content:center;border-radius:12px;overflow:hidden;">
        <img src="./images/firmware_client.png" style="max-width:100%;max-height:100%;width:auto;height:auto;display:block" alt="Firmware Upgrade Interface" />
    </div>
    </div>
    <div style="display:flex;gap:15px;justify-content:center;align-items:center;margin-bottom:20px">

    <div style="flex:1;display:flex;align-items:center;justify-content:center;border-radius:12px;overflow:hidden;box-shadow:0 4px 12px rgba(0,0,0,0.1);background:rgba(255,255,255,0.3);min-height:200px">
        <img src="./images/wave_client2.png" style="max-width:100%;max-height:100%;width:auto;height:auto;display:block" alt="Waveform Interface 2" />
    </div>

    <div style="flex:1;display:flex;align-items:center;justify-content:center;border-radius:12px;overflow:hidden;box-shadow:0 4px 12px rgba(0,0,0,0.1);background:rgba(255,255,255,0.3);min-height:200px">
        <img src="./images/set_client.png" style="max-width:100%;max-height:100%;width:auto;height:auto;display:block" alt="Settings Interface" />
    </div>

    </div>

## 📂 Project Structure
```text
Power-Pico/
├── Firmware/        # Firmware, including BootLoader.hex and App.bin
├── Hardware/        # Schematics
├── 3D models/       # STL files for 3D printing the enclosure
└── Software/        # MDK software project for PowerPico, including BootLoader and App
```

## 💻 Code Framework

Similar to previous projects, it uses a BootLoader + App structure for easy USB upgrades. The program partitioning is shown below.

<ul>
<li>The initial 48kB area of sectors 0, 1, and 2 is the BootLoader area.</li>
<li>The 16kB area of the 3rd sector is the upgrade information area, used for verifying the integrity of the App on startup. For specific verification details, see the BootLoader code.</li>
<li>The area starting from the 4th sector at 0x08010000 is the App area (A/B partitioning is not used due to insufficient on-chip Flash).</li>
</ul>
<div style="margin-top:20px;text-align:center">
    <img src="./images/flash.png" style="width:75%;border-radius:16px;box-shadow:0 8px 24px rgba(0,0,0,0.1);display:inline-block" alt="Flash Partitioning" />
</div>

High-speed data transmission is performed via USB. The data packet structure is shown below. Each packet contains 100 data sets, totaling 711 bytes including the header.
<div style="margin-top:20px;text-align:center">
    <img src="./images/protocol.png" style="width:100%;border-radius:10px;box-shadow:0 8px 24px rgba(0,0,0,0.1);display:inline-block" alt="Data Packet Structure" />
</div>

📖 References

<ul>
    <li style="margin-bottom:8px"><a href="https://github.com/ryan-ma/PD_Micro">[1] PD_Micro - USB-C PD3.0 PPS on ATMega32U4 Arduino board</a></li>
    <li style="margin-bottom:8px"><a href="https://wiki.luatos.com/iotpower/cc/index.html">[2] IoT Power-CC 3D printed shell structure</a></li>
    <li style="margin-bottom:8px"><a href="https://doc.qt.io/qtforpython-6/PySide6/QtWidgets/index.html">[3] PySide6 user manual</a></li>
    <li style="margin-bottom:8px"><a href="https://qfluentwidgets.com/zh/pages/about">[4] QFluentWidgets - Make QT Great Again</a></li>
    <li style="margin-bottom:8px"><a href="https://pyqtgraph.readthedocs.io/en/latest/getting_started/index.html">[5] PyQtGraph user manual</a></li>
    <li style="margin-bottom:8px"><a href="https://www.st.com/en/development-tools/stsw-stm32102.html">[6] STM32 USB vitual serial port driver demo</a></li>
    <li style="margin-bottom:8px"><a href="https://github.com/lvgl/lvgl">[7] LVGL - Embedded graphics library </a></li>
</ul>

🤝 Contribute

Welcome to submit Issues or Pull Requests to add new features to the project or fix code bugs!!
