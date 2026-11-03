# STM32F429 openedv 单工程多任务固件计划与验收记录

## 1. 目标
在 `D:/work/embeded_v3/base` 中实现一个面向 `openedv_stm32f4` 的单工程多任务固件，采用分层架构，使用 STM32 HAL、FreeRTOS、FatFs、LittleFS、EasyLogger、letter-shell、CherryUSB，并完成构建与 HIL 验收。

## 2. 架构与关键决策
- 目录分层：`platform/common/drv/os/bsp/mw/integration/app/third_party`。
- `cmsis_device` 为 OBJECT 库；HAL 驱动只引用 `third_party/stm32_hal` 源文件。
- `freertos_core` 不依赖 port；`freertos_port` 依赖 `cmsis_core` 和 `freertos_config`；`freertos` 为聚合 FACADE。
- `mw` 采用 `xxx_core + xxx_port + xxx_config`，通过 OBJECT 库和 `INTERFACE_SOURCES` 避免静态库循环依赖。
- 配置与适配分离：HAL/FreeRTOS/CherryUSB/FatFs/LittleFS/EasyLogger/letter-shell/LVGL 配置位于 `integration/openedv_stm32f4/config`。
- 不引入 `mw_fs` 抽象；FatFs 与 LittleFS 独立。USB MSC 直接访问 SD 块设备，FatFs 与 MSC 通过 `bsp_sd` owner 互斥。
- USB：CherryUSB DWC2 device；CDC + MSC 复合设备；无 USB Host 源；`HAL_PCD` 开、`HAL_HCD` 关。
- 时基：HAL tick/delay 使用 TIM6（84 MHz，1 kHz）；FreeRTOS tick 使用 SysTick；运行统计使用 TIM7（10 kHz）。
- letter-shell 使用静态命令表，不使用命令导出段和链接脚本修改。
- 看门狗：IWDG 约 4 s；`task_wdg` 等待各任务心跳位后喂狗；调试构建冻结 IWDG。

## 3. 主要模块
- `bsp_core`：168 MHz 时钟、TIM6 时基、IWDG、NVIC。
- `bsp_uart`：USART1 日志/Shell，USART2 周期帧；TX DMA、RX IT。
- `bsp_led/key/pwm/adc`：LED、按键、TIM8 PWM、ADC1 + DMA2 Stream4。
- `bsp_nor`：SPI5 W25Q NOR；`bsp_sd`：SDIO + FatFs 磁盘 IO + MSC owner。
- `bsp_oled`：SPI1 SSD1306。
- `mw_littlefs`：NOR 上的 LittleFS，日志文件后端起停。
- `mw_fatfs`：SD 卡 FAT 文件系统门面。
- `mw_cherryusb` + `mw_usb_bridge`：CDC 回显、MSC 原始 SD、FatFs/MSC 互斥。
- `task_ctrl/adc/wdg/log/oled/shell/usb` + 10 ms tick 分发器。

## 4. 构建
```bash
cmake --preset debug
cmake --build --preset debug

cmake --preset release
cmake --build --preset release
```
最终构建结果：
- Debug：`build/debug/firmware.elf`、`.bin`、`.map`；RAM `96128 B / 192 KB (48.89%)`，FLASH `292400 B / 1 MB (27.89%)`。
- Release：`build/release/firmware.elf`、`.bin`、`.map`；RAM `96152 B / 192 KB (48.91%)`，FLASH `308552 B / 1 MB (29.43%)`。
- Release 构建仅保留第三方 LittleFS 的 `lfs_mlist_isopen` 未使用警告，无本项目错误。

## 5. HIL 验收记录
测试环境：
- OpenOCD 0.12.0；
- CMSIS-DAP v2，VID:PID `0xfaed:0x4873`；
- 目标 STM32F4，1 MB Flash；
- 调试器 UART 对应 Windows `COM19`。

已完成：
- OpenOCD `program ... verify reset` 成功，Flash 校验通过。
- Debug 固件启动后进入 FreeRTOS 调度器，任务运行，`BASEPRI=0`。
- ADC 任务持续更新 6 路 mV 值（实测约 `537/641/3299/2461/19/17 mV`）。
- 通过调试器向 Shell RX ring 注入命令，验证 Shell 命令表、PWM 和按键通知：
  - `pwm 700`：PWM 从 500 变为 700；
  - `key 1`：PWM 从 700 变为 800，`last_key=1`。
- LittleFS：执行 `lfs format` 后 `g_mounted=1`，NOR + LittleFS + 日志文件后端可用。
- USB：`usb_connected=1`，USB Host 已实际读取 MSC 扇区，`g_msc_ready=1` 且 SD owner=MSC，MSC 已成功触发 FatFs/MSC 互斥切换。
- Release 固件运行超过 7 s，tick 约 7230 ms 且无复位，说明 `task_wdg` 心跳与 IWDG 喂狗工作正常。

未完成/受限：
- `COM19` 未连接到 USART1/2：固件 USART1/2 的 TX 队列已排空且 USART TC 置位，但 PC 收不到数据，无法进行完整串口 Shell/日志 HIL。需要把 CMSIS-DAP UART 接到 USART1 PA9/PA10，或改用 USB CDC。
- FatFs 挂载未验证：当前 USB Host 正在访问 MSC，MSC 持有 SD owner，FatFs 按设计无法同时挂载。需要在 USB 断开后用 FAT 格式 SD 卡验证。
- USB MSC 读写、CDC 主机回显、OLED 实际显示、硬件按键和 IWDG 复位路径仍需在完整硬件条件下补充测试。

## 6. 启动期中断屏蔽问题与处理
FreeRTOS 11.3.3 的 `uxCriticalNesting` 在调度器启动前为 `0xaaaaaaaa`。在调度器启动前创建/使用互斥量、队列等对象时，临界区退出不会恢复 `BASEPRI`，导致 TIM6 中断被屏蔽，`HAL_Delay()` 卡死。

处理：
- `app_init` 在各启动阶段后调用 `irq_unmask()`（清 `BASEPRI` 并开 IRQ）；
- `bsp_sd_init` 创建互斥量后立即恢复中断；
- `mw_littlefs_lock`、`mw_fatfs_lock`、`elog_port_output_lock` 在调度器未启动时使用非阻塞超时。
- `usbd_msc_get_cap` 不再提前卸载 FatFs/获取 SD owner；USB configured 仅标记 MSC active，只有实际 MSC 扇区读写才触发 FatFs/MSC 切换。

后续建议：将 LittleFS/日志/USB 等依赖 FreeRTOS 对象和阻塞操作的初始化迁移到启动任务中，彻底移除 `irq_unmask()` 兼容处理。

## 7. 后续工作
1. 连接 UART 或使用 USB CDC，完成串口 Shell 全命令验收。
2. 断开 USB，使用 FAT 格式 SD 卡验证 `sd mount/ls/umount` 与 FatFs 读写。
3. 验证 MSC 读写、CDC 回显、OLED 显示、硬件按键、IWDG 超时复位。
4. 评估将启动期初始化迁移到 `task_boot`，消除 FreeRTOS 调度器启动前临界区问题。
