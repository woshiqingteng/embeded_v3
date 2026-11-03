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

## 7. 当前问题修复计划：USB MSC 挂载不可读
当前现象：USB 已连接，PC 能看到 Mass Storage 设备，但 U 盘/盘符无法正常读取。USART1 已连接，USB 已连接；串口调试链路视实际 COM 口可用性用于抓取日志。

### 7.1 复现与证据采集
- 用 USART1/COM 口确认固件启动日志、Shell 提示符和 `usb`/`sd` 命令输出。
- 记录 PC 端枚举结果：VID/PID、容量、盘符、读写错误。
- 用调试器读取：`g_ready`、`g_owner`、`g_msc_ready`、`bsp_sd_get_info()` 的 `block_count/block_size`、USB MSC 容量回调返回值。
- 记录 USB MSC 的 `usbd_msc_sector_read/write` 调用次数、扇区号、长度和返回值。

### 7.2 SD 卡与块设备路径排查
- 验证 `sdio_init()`、SD 卡类型/容量、4-bit 总线、时钟分频和读写单扇区/多扇区。
- 验证 `bsp_sd_acquire()`/`bsp_sd_release()` 的 owner 切换，确保 MSC 获取后 `bsp_sd_read()` 返回成功。
- 验证空卡、非 FAT 卡、FAT 卡三种情况下的容量与原始扇区读取。
- 如 SD 初始化成功但容量为 0 或读失败，修正 SDIO 初始化/卡信息获取/读写实现。

### 7.3 MSC 类与 USB 描述符排查
- 核对 `usbd_msc_get_cap()` 返回的 block count/size，确认主机看到的容量非 0。
- 核对 MSC Bulk IN/OUT endpoint 地址、最大包长、双缓冲和 `CONFIG_USBDEV_MSC_MAX_BUFSIZE`。
- 核对 CDC+MSC 复合描述符、IAD/接口编号、配置总长度和端点分配。
- 检查 `usbd_msc_sector_read/write` 的返回值、长度校验和缓冲区生命周期。
- 修正 FatFs/MSC 互斥逻辑，保证 MSC 首次扇区访问前完成 unmount/acquire，并避免并发访问。

### 7.4 修复与回归
- 优先保证只读：PC 能读取容量、分区表和文件系统/原始扇区。
- 再验证写入、读回、拔插、重复挂载和 FatFs/MSC 切换。
- 验证 USB CDC 回显、USART1 Shell、LittleFS、ADC/PWM/按键、看门狗。
- Debug/Release 全量构建，OpenOCD 烧录校验，连续运行无 HardFault/复位。

### 7.5 验收标准
- PC 能稳定识别 MSC 容量，不再显示 0 字节或读取失败。
- 主机可读取 SD 卡分区/文件系统；写入后读回一致。
- USB 断开后 `sd mount`/`sd ls` 可用；USB 接入时 MSC/FatFs 不并发冲突。
- USART1 Shell 命令全部可用，USB CDC 回显可用。
- Debug/Release 构建无本项目错误；HIL 运行无 HardFault、无意外 IWDG 复位。

## 8. 后续工作
1. 完成上述 USB MSC 修复与全量 HIL。
2. 断开 USB，使用 FAT 格式 SD 卡验证 `sd mount/ls/umount` 与 FatFs 读写。
3. 验证 OLED 显示、硬件按键和 IWDG 超时复位。
4. 评估将启动期初始化迁移到 `task_boot`，消除 FreeRTOS 调度器启动前临界区问题。
