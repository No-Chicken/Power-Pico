# CMake / GCC 构建与 PR #6 修复来源

## 来源与范围

本次构建及内存布局修复基于 EudaiG 的 PR #6 首个提交：
https://github.com/No-Chicken/Power-Pico/pull/6
https://github.com/EudaiG/Power-Pico/commit/d5f2f644e38a072c6729e283662276474949d291

采用其源码收集修正、NuttX 排除、浮点 printf、FreeRTOS GCC ARM_CM4F port、链接栈布局及断言、LVGL 缓冲配置。FreeRTOS port 保留上游许可证。
不包含该 PR 后续的 PD、诊断、EEPROM、任务调整或 UI 改版，也不引入 Modern/Classic、LTO、EEPROM 恢复选项及作者本机 Keil 配置。
额外集成修正仅为三处缺失声明/include 和 ui_ChartPage.h 文件名大小写。修正了原提交中关于 Keil 栈大小及缓冲字节数的不准确注释。

## 编译

安装 CMake >= 3.22、Ninja 和 ARM GNU Toolchain，将其 bin 目录加入 PATH，在本目录执行：

```powershell
cmake --preset MinSizeRel -DCMAKE_C_FLAGS=-Werror=implicit-function-declaration
cmake --build --preset MinSizeRel
```

也验证了 Release。产物为 build/MinSizeRel/output/power_pico.bin，调试符号文件为 build/MinSizeRel/power_pico.elf（该配置默认不含完整源码调试信息）。Debug 使用 -O0，不能据此推断能够装入 Flash；本次未验证 Debug。

## 固件与内存

- APP Flash 起始地址：0x08010000，容量 448 KiB；bin 是 APP，不含 Bootloader。
- 初始 MSP：0x2001D820；主栈最低预留 0x2800（10 KiB）。这不改变 FreeRTOS 任务栈。
- 链接断言检查栈顶及静态数据/堆与预留主栈区域。链接检查不替代运行时栈余量检查。
- LVGL：原 57600 字节单缓冲改为两个各 19200 字节的缓冲，总计 38400 字节，节省 19200 字节。
- LVGL 文件为 GCC/Keil 共用，因此未来 Keil 构建也采用此缓冲设置；本次没有重新验证 Keil。

## 2026-10-24 验证记录

工具链 xPack ARM GCC 14.2.1-1.1。Release / MinSizeRel 均构建成功，0 error、0 隐式声明，仍有 159 条其他警告，未作为本次范围扩展处理。
Flash 402852 / 458752 字节（87.81%）；链接器报告 RAM 112360 / 131072 字节（85.72%，含链接预留，不代表实际最大栈用量）。

两种配置生成相同 bin，SHA256：
FC6C5966AAD5B09FC2999397D03B3865BAC0E7A3D9A90B747BB958B977A19A2E

用户已报告原始 c1 对照固件可以经现有 Bootloader 启动。对照固件 SHA256：
BCF3B4820C0E8159A06C25A3ADC218A4F41D445705A4E1031B5296215012F98C

正式版本与该对照固件有 202 个字节不同，全部位于 _setting_timer_cb 内 0x0803B732..0x0803B7FB。补充 int8_t 返回类型声明后，GCC 消除一次多余符号扩展并调整该函数内部指令；其余 bin 字节一致。正式版本仍需上板确认，不能将对照版本的结果当作正式版本完整验收。

## 上线前回归

- 正式 bin 经 Bootloader 升级后冷启动、热复位和重复启动。
- 设置页、PD 等待/成功/失败提示、页面切换、屏幕旋转与连续刷新，检查双缓冲是否花屏或卡死。
- 浮点数值显示、USB 数据传输、原有 PD 功能、设置和校准值保持。
- GCC 与现有 Keil 固件双向 OTA；如发布 Keil 固件，单独构建并上板验证。
- 长时间采集和任务栈高水位检查。

目前证据支持这组改动可改善启动，尚未单独证明固定 _estack 是原故障根因，也未证明全部硬件场景均已通过。
