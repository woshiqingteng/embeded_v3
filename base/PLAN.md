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
- USART1 已接入 CMSIS-DAP UART（COM19），Shell 启动横幅和 `sdraw`/`usb` 命令已验证。
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

## 8. 长时间未解决问题记录（避免重复试错）
### 8.1 USB MSC 大文件写入/读取后卡死、盘符消失
现象：
- 小文件（4 KB、64 KB、128 KB、1 MB）可读写；
- 256 KB/1 MB/8 MB 某些操作后 PC I/O 挂起，盘符消失，串口出现重复启动横幅；
- 调试器偶尔看到 PC 在 `Reset_Handler`，说明发生了复位而不是单纯 USB 断开。

已确认的根因/证据：
- SDIO 原始扇区读写在 16 MHz 下偶发失败；降到 8 MHz 后改善，最终固定为 4 MHz 以提高稳定性。
- `SD_TIMEOUT_COUNT` 原为 `100000000`，作为 HAL 超时（ms）过大，卡异常时主机长时间等待。
- `task_wdg` 优先级为 8，CherryUSB MSC 线程优先级为 11；大文件连续写入时 MSC 线程可长时间占用 CPU，`task_wdg` 无法运行并喂狗，IWDG 约 4 s 后复位。
- `WDG_BIT_STORAGE` 已定义但未参与 `WDG_BIT_ALL`，也没有在存储 I/O 路径中喂狗。

已实施：
- SDIO 传输时钟调整为 4 MHz。
- HAL SD 超时改为 2 s，状态等待使用独立计数，读写各重试 3 次。
- 保留 `sdraw` 原始扇区诊断命令，用于验证 SD 块层。
- 暂未完成：存储 I/O 路径喂狗；大文件回归；FatFs 在 USB 断开后的验证。

### 8.2 仍需验证
- 8 MB 及以上持续写入/读取不触发 IWDG 复位。
- USB 断开后 FatFs 挂载/读写；USB 接入时 MSC/FatFs 切换。
- CDC 回显、OLED、硬件按键。
- Debug/Release 使用 `.bin` 烧录的完整回归。

## 9. CMake 与运行时重构计划（2026-11-03）
### 9.1 单一版本清单
- 新增单一文件 `cmake/third_party/dependencies.cmake`，集中定义第三方版本与 `TP_*` 路径。
- 所有 CMake 从版本清单拼路径，不再在子目录散落版本号。
- `stm32f4xx_hal_conf.h` 默认模块全部注释；由 `drv_hal` 的 CMake 通过编译定义启用实际模块。

### 9.2 CMake 目录与目标
- 删除 `integration/**/CMakeLists.txt`；`integration` 仅保留配置/板级适配源码。
- 每个 mw 一个单一 STATIC target：`mw_cherryusb`、`mw_fatfs`、`mw_littlefs`、`mw_easylogger`、`mw_lettershell`、`mw_lvgl`。
- `mw_cherryusb` 内注释区分 core/class/osal/port/板级回调，不再拆子 target。
- 板级适配源通过 `BOARD_*_SOURCES` 注入：
  - `BOARD_CHERRYUSB_SOURCES`
  - `BOARD_FATFS_SOURCES`
  - `BOARD_LITTLEFS_SOURCES`
  - `BOARD_EASYLOGGER_SOURCES`
  - `BOARD_LETTERSHELL_SOURCES`
  - `BOARD_FREERTOS_SOURCES`
- `CMakePresets.json` 保留 `BOARD`，由 board cmake 自动解析 MCU、OpenOCD、板级源和配置路径。
- 合并 `drv_core/drv_uart/drv_tim/drv_adc/drv_spi/drv_iwdg/drv_sd` 为单一 `drv_hal` STATIC target，直接编译 `third_party/stm32_hal`。
- `DRV_BACKEND=hal` 作为当前后端；HAL conf 由 `drv_hal` 启用。
- FreeRTOS `port` 保持独立 target；core/port 仍可分离编译。
- 去除 `integration` 相关 CMake target；由 mw 直接引入板级源。
- 链接脚本、syscalls 从 `cmsis_device` 移出：
  - `cmsis_startup`：startup/vector/system/it
  - `platform_linker`：链接脚本/Map 选项，仅链接一次
  - `platform_syscalls`：newlib syscall stubs
- 引脚宏只放对应 `.c`，公共头文件不暴露引脚宏（先处理 `sdio.h`）。

### 9.3 启动与运行时
- 删除 `app_init` 的 `irq_unmask()` 和 `bsp_sd` 的 `__set_BASEPRI`/`__enable_irq`。
- 通过启动任务（`task_boot`）在调度器启动后完成 FS/SD/OLED/Shell/USB/IPC 初始化，避免调度器前 FreeRTOS 临界区问题。
- tick 从软件定时器回调改为独立 `task_tick`（`vTaskDelayUntil` 10 ms）。
- 看门狗重构：
  - `task_wdg` 提高优先级；
  - 存储 I/O 提供显式心跳；
  - 明确 heartbeat 位要求，避免 MSC 线程饿死看门狗。
- newlib/printf 线程安全：保留 `configUSE_NEWLIB_REENTRANT=1`，并保证 `_write`/UART 输出串行化；必要时加 newlib 锁。
- 故障可诊断：实现 `HardFault_Handler`，保存 CFSR/HFSR/MMFAR/BFAR、PC/LR/PSP/MSP，并通过日志/UART输出或保存到 no-init RAM。
- OLED 并发：增加递归锁，`task_oled` 与 Shell `oled` 命令互斥。
- SDIO 使用 DMA + 4-bit 总线，替换阻塞轮询；增加超时/重试/心跳。

### 9.4 验收
- `cmake --preset debug`/`release` 均可配置与构建。
- 使用 `.bin` 烧录验证：UART1 Shell、USB MSC 读写、FatFs/MSC 切换、LittleFS、ADC/PWM/按键、OLED、IWDG。
- 大文件 MSC 读写不触发 HardFault/IWDG 复位；故障时能输出可诊断信息。

## 10. CMake/端口重构完成状态（2026-11-03）
已完成：
- 删除 `board/` 与 `integration/`，板级适配按 mw 归类到 `port/openedv_stm32f4/`：
  - `cherryusb/`：`cherryusb_class_cb.c`、`usb_config.h`
  - `fatfs/`：`diskio_sd.c`、`ffconf.h`
  - `littlefs/`：`lfs_bd_nor.c`、`lfs_port.h`、`lfs_config.h`、`lfs_alloc.h`
  - `easylogger/`：`elog_port.c`、`elog_cfg.h`
  - `lettershell/`：`shell_port.c`、`shell_cfg.h`
  - `freertos/`：`FreeRTOSConfig.h`、`freertos_glue.c`、`runtime_stats_tim7.c`
  - `lvgl/`：`lv_conf.h`
- 非 port 内容移出：`usb_bridge.c/.h` 移入 `mw/cherryusb/`，`log_file.c` 移入 `mw/easylogger/`。
- 所有 mw 为单一 STATIC target；`freertos_port` 独立 target；`integration` 无 CMake target。
- `BOARD_*_SOURCES` 注入板级 port；`CMakePresets` 保留 `BOARD`/`DRV_BACKEND=hal`。
- 删除 `dma_hw.h`、`gpio_hw.h`、`irq_unmask()`、bsp `BASEPRI`、`.vscode` `APP_BANNER`。
- `SysTick_Handler` 通过 `FreeRTOSConfig.h` 的 `xPortSysTickHandler` 映射定义。
- 链接脚本移至 `platform/linker`，syscalls 移至 `platform/syscalls`；`cmsis_startup`、`platform_linker`、`platform_syscalls` 仅 app 链接一次。
- 启动改为 `task_boot`：调度器启动后初始化 SD/FS/OLED/Shell/IPC/tasks；删除调度器前 BASEPRI 兼容代码。
- tick 改为独立 `task_tick`（10 ms）；看门狗任务优先级提高，存储 I/O 直接心跳喂狗；OLED 增加递归锁；HardFault 保存 CFSR/HFSR/PC/LR 等并复位后可诊断。
- Debug/Release 构建通过；Debug `.bin` 烧录验证启动、USART1 Shell、`sdraw 0`、USB 配置。

未完成/风险：
- `sdio.c` 已有 DMA 路径和 4-bit 配置，但 DMA 实测启动后无 DMA 完成中断；当前设置 `SDIO_USE_DMA 0`，实际使用 4-bit 阻塞读写。需继续定位 DMA 流映射/中断原因并启用。
- PC 侧 1 MB 文件写入仍会卡住，目标板未复位；需在启用/修复 DMA 后复测，并确认 Windows 卷缓存/清理状态。
- 大文件 MSC 读写的完整 HIL、FatFs 断开 USB 后复测、CDC 回显、OLED 实际显示仍需完成。

## 11. 最终清理与验证状态（2026-11-03）
- 已删除全部 LVGL 内容：`third_party/lvgl`、`mw/lvgl`、`port/openedv_stm32f4/lvgl`、LVGL CMake/版本/配置引用；CherryUSB 的 vendor `platform/lvgl` 也已移除。
- 已彻底改为 tick 任务：删除 `timers.c`、关闭 `configUSE_TIMERS`，移除软件定时器配置；`task_tick` 使用 `vTaskDelayUntil(10 ms)`。
- `runtime_stats_tim7.c` 内容迁入 `bsp_core/bsp_runtime_stats.c`；删除 `port/freertos/freertos_glue.c`，FreeRTOS hooks 移入 `app/system/freertos_hooks.c`；`port/freertos/` 仅保留 `FreeRTOSConfig.h`。
- LittleFS port 已缩减：删除 `lfs_port.h` 和空 `lfs_port_init()`，配置声明合并到 `lfs_config.h`。
- 识别 SD 卡写失败/只读：启动时对 SD 最后一个未使用扇区做写入自检；失败则通过 `usbd_msc_set_readonly()` 将 MSC 暴露为只读。实测 PC 写入返回 `Permission denied`，不再挂起；盘符/文件读取正常。
- SDIO DMA 仍保留代码但默认 `SDIO_USE_DMA=0`；DMA 完成中断未验证，当前 4-bit 阻塞路径可用。
- Debug/Release 构建通过：Debug FLASH `~296 KB`，Release FLASH `310632 B`。

仍待完成：
- 若需要 U 盘可写，需要更换/解锁 SD 卡后验证；当前卡写入在所有测试 LBA 均失败。
- SDIO DMA 完成中断、MSC 大文件写入、FatFs 断 USB 后复测、CDC 回显、OLED 实显仍需硬件回归。

## 12. system/app 初始化拆分（2026-11-03）
- 新增 `system_init()`：负责 BSP 初始化、MW 初始化、系统任务（`task_wdg`、`task_shell`、`task_log`、`task_usb`、`task_tick`）以及 `app_ipc_init()`。
- `app_init()` 只负责应用任务：`task_ctrl`、`task_adc`、`task_oled`，并初始化应用状态。
- `main()` 调用 `system_init()`；系统启动任务在调度器启动后完成 BSP/MW/system 初始化，再调用 `app_init()` 创建应用任务。
- tick 已无软件定时器（`configUSE_TIMERS=0`，`timers.c` 不再编译）。

## 13. 未完成项收尾计划（2026-11-03）
1. SDIO DMA 修复
   - 在启动阶段、USB Host 尚未访问 SD 前做一次 DMA 读自检，排除 MSC 并发干扰。
   - 自检通过则运行时使用 DMA；失败则保持 4-bit 阻塞回退，并记录原因。
   - 核对 DMA2 Stream3/6、Channel4、NVIC、SDIO IRQ、DMA 完成回调。
2. U 盘只读与读性能
   - 当前 SD 卡所有 LBA 写测试失败，保持 MSC 只读；验证盘符、文件读取和大块顺序读取。
   - 若需要写入，要求更换/解锁 SD 卡后重新验证，不在软件层伪造写入成功。
3. 系统任务与应用任务
   - 保持 `system_init` 初始化 BSP/MW/系统任务，`app_init` 只创建应用任务。
4. 回归
   - Debug/Release 构建。
   - USART1 Shell、USB 识别、MSC 只读读取、LittleFS、ADC/PWM/按键、IWDG、OLED 任务。
   - 记录仍受硬件限制的项目（SD 卡写保护、DMA 若自检失败）。

## 14. 收尾结果（2026-11-03）
- SDIO DMA：已核对 DMA2 Stream3/6 Channel4、NVIC、SDIO IRQ 和 4-bit 配置；增加启动期 DMA 读自检后自检失败/阻塞，故保持 `SDIO_USE_DMA=0`，运行时使用 4-bit 阻塞读写。DMA 路径保留但不启用。
- U 盘：SD 卡所有测试 LBA 写入失败，启动写自检判定为只读；MSC 通过 `usbd_msc_set_readonly()` 暴露为只读。PC 端已验证分区/目录可读，写入返回 `Permission denied`，不再挂起。
- Debug/Release 均构建通过；当前 Release FLASH `310580 B`。
- 仍受硬件限制：SD 卡写保护/写失败；若需可写 U 盘，需更换或解锁 SD 卡。SDIO DMA 4-bit 仍需在确认 DMA 完成中断原因后启用。

## 15. 板级驱动的 CMake 编排（2026-11-03）
- `CMakePresets.json` 只设置 `BOARD`（不再设置 `DRV_BACKEND`）。
- `cmake/board/openedv_stm32f4.cmake` 确定 `MCU STM32F429IGTx`、`DRV_BACKEND stm32f4_hal`、`DRV_TARGET drv_hal`，并引入板级 port 源/配置。
- `cmake/chip/stm32f429igtx.cmake` 保存芯片属性（架构、宏、HSE/HSI、启动模式），由 board 文件自动引入；板子确定芯片。
- `drv/CMakeLists.txt` 只做编排：检查 `DRV_BACKEND`，`add_subdirectory(${DRV_BACKEND})`，生成 `drv` 接口。
- `drv/stm32f4_hal/CMakeLists.txt` 保存具体 HAL 源文件、包含路径和模块开关，不再放在 `drv/CMakeLists.txt`。
- Debug/Release 构建已通过。

## 16. app 层整理（2026-11-03）
- 目录拆分：
  - `app/` 根：应用状态、`app_init`；`app/task/` 放应用任务 `task_app_ctrl/task_app_adc/task_app_oled`
  - `app/system/`：`system_init`、IPC、tick、fault、FreeRTOS hooks、Shell 命令
  - `app/task/` 系统任务 `task_sys_wdg/task_sys_log/task_sys_shell/task_sys_usb`
- `app/CMakeLists.txt` 使用明确的 app/system 源和 include 路径，不再共用 `app/task`。
- 移除 Shell 中纯调试命令 `sdraw/sdwrite/sdstat/msc` 及对应 extern，减少 app 层无关内容。

## 17. 剩余任务执行计划（2026-11-03）
### 17.1 目标
- 完成 SDIO 4-bit DMA 路径的原因定位与修复；无法修复时明确保留阻塞回退。
- 验证 U 盘只读读取、系统/app 任务运行、IWDG 和基本外设。
- 完成 Debug/Release 构建与 `.bin` HIL 记录。

### 17.2 步骤
1. 基线检查：当前源码结构、构建产物、启动日志。
2. SDIO DMA 复现：启用 `SDIO_USE_DMA=1`，启动期自检，抓取 PC、SDIO/DMA 寄存器、信号量状态，确定卡死位置。
3. 修复或回退：修正 DMA 配置/中断/回调；若硬件或 HAL 限制，保持 `SDIO_USE_DMA=0` 并删除无效自检路径。
4. U 盘验证：分区/目录/文件读取；只读写入返回错误；记录 SD 卡写保护限制。
5. 任务验证：确认 system/app 任务均启动、看门狗不误复位。
6. 构建回归：Debug/Release，更新 PLAN。

## 18. SDIO DMA 修复结果（2026-11-03）
- 根因：DMA 完成回调在中断上下文调用 `xSemaphoreGive()`，进入 FreeRTOS 临界区后卡死；改为 `xSemaphoreGiveFromISR()` + `portYIELD_FROM_ISR()`。
- 修复后启动期 DMA 读自检通过，但主机侧 MSC 回归未通过；最终恢复 `SDIO_USE_DMA=0`，当前使用 4-bit 阻塞路径。
- U 盘仍按 SD 卡写失败设为只读；当前主机 USB 未重新枚举，需物理重新插拔后复测 MSC/DMA 大文件读取。
- Debug/Release 构建通过。

## 19. platform 启动/链接/syscalls 拆分（2026-11-03）
- `syscalls.c` 不属于 platform 硬件抽象，已移动到 `app/system/syscalls.c`，作为 app 源文件只链接一次。
- `cmsis_device` 改为纯头文件 INTERFACE target，只暴露芯片厂商 Include；可被多个库重复链接，不会带入启动/链接对象。
- 启动文件移到 `platform/startup/stm32f4xx/`，由独立 `cmsis_startup` OBJECT target 编译；仅 app 链接一次。
- 链接脚本按板子放到 `platform/linker/openedv_stm32f4/`，由 board 设置 `BOARD_LINKER_SCRIPT`；`platform/linker` 上层仅保留 CMake 编排，`platform_linker` 仅 app 链接一次。
- `cmake/chip/stm32f429igtx.cmake` 设置 `MCU_CMSIS_DEVICE`，实现板子 -> 芯片 -> CMSIS device 绑定。
- `CMakeLists.txt` 先加载第三方依赖清单，再加载板子/芯片配置。
- Debug/Release 均构建通过；`.bin` 启动验证 `BSP + MW + system/app tasks ready`，`stats` 显示 10 个任务、USB 已配置。

## 20. 第三轮整理结果（2026-11-03）
- FreeRTOS 不再区分 core/port：
  - 删除 `os/freertos/core/`、`os/freertos/port/`；
  - 合并为单一 `os_freertos` STATIC target，统一暴露给上层；
  - 上层（app/BSP/MW）全部改链接 `os_freertos`。
- SD 安全修正：
  - 删除会写最后一个 LBA 的破坏性可写测试；
  - 默认 `BSP_SD_WRITABLE=0`，通过 `bsp_sd_is_writable()` 暴露只读状态；
  - `bsp_sd_read/write` 的 owner 检查移入 mutex，`bsp_sd_owner()` 加锁；
  - USB MSC 默认只读，不再启动时写卡。
- CMake/分层：
  - `BSP_BACKEND` 由 board 提供，`bsp/CMakeLists.txt` 只做编排；
  - BSP/MW 统一链接 `drv` 接口，不再直接绕过到 `drv_hal`；
  - startup 源文件列表下沉到 `platform/startup/stm32f4xx/CMakeLists.txt`，上层不写死器件文件名；
  - 清理 app 的失效 include 路径和重复 BSP 链接；
  - 增加 `BSP_BACKEND`/`MCU_CMSIS_DEVICE` 检查、路径引号；
  - 删除未使用的 `MCU_TARGET_DIR`/`MCU_LD_SUFFIX`。
- 新增 `.gitignore`（build/、bin/elf/map 等）。
- Debug/Release 已构建通过（Release FLASH 约 307.5 KB）。
- HIL 复测结果见第 21 节。

## 21. 当前 HIL 复测记录（2026-11-03）
- Debug/Release 构建通过。
- Debug 启动：`BSP + MW + system/app tasks ready`，`stats` 任务数 10。
- Release 连续运行超过 9 s，`uwTick=13673`，未发生 IWDG 复位。
- USB 已重新枚举：`VID_0483&PID_5740`，`usb configured=1`。
- U 盘 `I:` 可读 `test.txt` / `FATFS.TXT`；写入返回 `PermissionError(13)`，只读策略有效。
- 清除 RCC 复位标志后 Release 连续运行约 13 s，`uwTick=13451`，RCC_CSR 无 IWDG 复位标志。
- `lfs ls` 正常列出根目录与 `log`。
- 当前 SDIO 使用 4-bit 阻塞模式；DMA 自检代码保留但 `SDIO_USE_DMA=0`。
