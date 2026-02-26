<p align="right">
  <b>中文</b> | <a href="./README_EN.md">English</a>
</p>

<div align="center">
  <h1 align="center">Power-Pico</h1>
  <p align="center">一款产品级的便携式 USB 电流表与低功耗测量分析仪</p>

  <img src="https://img.shields.io/badge/Version-1.0.5-blue">
  <img src="https://img.shields.io/badge/License-Apache2.0-green">
  <img src="https://img.shields.io/badge/MCU-STM32F411-lightblue">
  <img src="https://img.shields.io/badge/UI-LVGL_v9.2-red">
  <img src="https://img.shields.io/badge/OS-FreeRTOS-green">
  <br>
  <img src="https://img.shields.io/github/stars/No-Chicken/Power-Pico.svg">
</div>

---

## 📖 项目简介

**Power-Pico** 不仅仅是一个 USB 电流表。它是专为嵌入式开发者设计的**高精度低功耗分析工具**。针对传统电流表在 $\mu A$ 级别测量时的物理局限性，Power-Pico 采用了创新的**多档位自动切换采样**设计，实现了从 $1\mu A$ 到 $5A$ 的全量程覆盖。

<p align="center">
	<a href="https://space.bilibili.com/34154740">bilibili主页 </a> |
	<a href="">演示视频 </a> |
    <a href="https://oshwhub.com/no_chicken/powerpico">硬件开源 </a>
</p>

<p align="center">
	<img border="1px" width="75%" src="./images/main2316.png">
</p>

### ✨ 核心特性

- **uA 级高精度**：采用 INA190 零漂移放大器，配合三档电阻分段采样。
- **协议诱骗**：支持 PD2.0 PPS 协议诱骗，最高支持 20V/5A 诱骗输出。
- **高性能 UI**：基于 STM32F411 + LVGL v9.2，提供丝滑的实时波形显示。
- **优雅上位机**：配套 PySide6 开发的 Power-Pico Client，支持数据导出与实时功耗分析。
- **通用Type-C**：全部接口都是Typec-C，可以方便的外接Type-C转鳄鱼头等接头。
- **3D打印外壳**：提供完整的方便用户组装的 3D 打印外壳与亚克力面板设计方案。

---

## 🛠️ 硬件参数分析

<div align="center">

| 模块 | 选型 | 优势 |
| :--- | :--- | :--- |
| **主控** | STM32F411CEU6 | 100MHz 高主频，支持 DSP 指令加速波形处理 |
| **电流采样** | INA190 | 0.5nA 输入偏置电流，μA 级测量的核心保证 |
| **ADC** | 12-bit (STM32 Internal) | 配合 DMA 实现 10kHz 以上的高速稳定采样 |
| **屏幕** | 1.54" TFT (ST7789) | 240x240 分辨率，高对比度显示 |

</div>
---

## 🚀 快速复刻

### 1.硬件
自行焊接硬件，或购买淘宝已经焊接组装好的套件
立创硬件开源界面：https://oshwhub.com/no_chicken/powerpico

### 2. 固件自行编译烧录
项目基于 Keil MDK 开发：
1. 克隆仓库：`git clone https://github.com/kingham/Power-Pico.git`
2. SWD口烧录BootLoader
3. 自行编译PowerPico，编译好生成的`.bin`文件再使用PowerPico Client烧录即可

### 3. 结构组装

实物组装如下图所示:

<p align="center">
	<img width="45%" src="./images/PCBA.jpg">
    &nbsp;
    <img width="45%" src="./images/3Dall.jpg">
</p>

3D爆炸图如下图所示:

<p align="center">
	<img width="45%" src="./images/boom.png">
    &nbsp;
    <img width="40%" src="./images/boom.gif">
</p>

### 4. 上位机运行
安装PowerPico Client使用即可，目前只支持window。

## 🛠️ 使用说明

1. 硬件操作说明

    Power-Pico的按键操作说明，和各个界面展示，如下图所示。顶部按键为 **换页/Boot** 按键，长按住顶部按键再上电，会直接进入Boot模式强制升级模式。

    Power-Pico硬件目前只支持 **中文简体/English** 2种语言。

    <p align="center">
        <img width="25%" src="./images/key_func.png">
        &nbsp;
        <img width="25%" src="./images/startup_page.png">
    </p>
    <p align="center">
        <img width="25%" src="./images/main_page.png">
        &nbsp;
        <img width="25%" src="./images/set_page.png">
    </p>
    <p align="center">
        <img width="25%" src="./images/pps_page1.png">
        &nbsp;
        <img width="25%" src="./images/pps_page2.png">
    </p>

2. 客户端操作说明

    PowerPico Client电脑客户端可以用于波形查看、记录、导出；能够进行对下位机硬件的固件升级。客户端目前支持 **中文简体/中文繁体/English/日本語** 4种语言。

    PowerPico Client客户端上位机的具体使用方法详见：[客户端操作说明](https://no-chicken.com/content/Power-Pico/UserManual/client_operation.html)

    <p align="center">
        <img width="45%" src="./images/wave_client1.png">
        &nbsp;
        <img width="45%" src="./images/firmware_client.png">
    </p>
    <p align="center">
        <img width="45%" src="./images/wave_client2.png">
        &nbsp;
        <img width="45%" src="./images/set_client.png">
    </p>

## 📂 项目结构
```text
Power-Pico/
├── Firmware/        # 固件，包括BootLoader.hex和App.bin
├── Hardware/        # 原理图
├── 3D models/       # STL文件，用于3D打印外壳
└── Software/        # PowerPico的MDK软件工程，包括BootLoader和App
```

## 💻代码框架

跟以前的项目一样，采用了 BootLoader + App 的结构，方便USB升级。程序分区如下图所示。

<ul>
<li>起始的sector0,1,2的48kB区域为BootLoader区域</li>
<li>第3个sector的16kB区域为升级信息区域，用于开机校检App完整性，具体的校检内容详见BootLoader代码</li>
<li>从0x08010000第4个扇区开始后的区域，都为App区域（之所以不做AB分区是片上Flash不够）</li>
</ul>

<p align="center">
    <img width="75%" src="./images/flash.png">
</p>

使用USB进行高速数据传输，数据包组成如下图所示。每个数据包100组数据，加上头共计711Bytes。

<p align="center">
    <img width="75%" src="./images/protocol.png">
</p>

## 📖 参考资料

<ul>
    <li style="margin-bottom:8px"><a href="https://github.com/ryan-ma/PD_Micro">[1] PD_Micro - ATMega32U4 Arduino板上的USB-C PD3.0 PPS</a></li>
    <li style="margin-bottom:8px"><a href="https://wiki.luatos.com/iotpower/cc/index.html">[2] 合宙 Power CC 外壳结构</a></li>
    <li style="margin-bottom:8px"><a href="https://doc.qt.io/qtforpython-6/PySide6/QtWidgets/index.html">[3] PySide6手册</a></li>
    <li style="margin-bottom:8px"><a href="https://qfluentwidgets.com/zh/pages/about">[4] QFluentWidgets - 让QT再次伟大</a></li>
    <li style="margin-bottom:8px"><a href="https://pyqtgraph.readthedocs.io/en/latest/getting_started/index.html">[5] PyQtGraph 使用手册</a></li>
    <li style="margin-bottom:8px"><a href="https://www.st.com/en/development-tools/stsw-stm32102.html">[6] STM32 USB 虚拟串口driver案例</a></li>
    <li style="margin-bottom:8px"><a href="https://github.com/lvgl/lvgl">[7] LVGL - Embedded graphics library </a></li>
</ul>

## 🤝 提交共享

欢迎提交 Issue 或 Pull Request 来增加项目功能，或修改代码bug!!
