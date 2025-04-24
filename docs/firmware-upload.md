# 固件上传

本文档详细介绍如何将编译好的固件上传到 LC76G_Pico 设备中。

## 固件文件类型

LC76G_Pico 项目生成的固件是 `.uf2` 格式的文件，这是 Raspberry Pi Pico 官方支持的固件格式。编译成功后，您可能会得到以下文件：

- `gps_example.uf2` - 基本 GPS 功能示例
- `vendor_gps_display.uf2` - 包含 GPS 和显示屏功能的综合示例

## 上传步骤

### 方法一：通过 BOOTSEL 按钮上传

这是最常用的上传方式，适用于大多数情况：

1. **进入 BOOTSEL 模式**：
   - 按住 Raspberry Pi Pico 上的 BOOTSEL 按钮
   - 在按住按钮的同时将 Pico 连接到计算机
   - 连接后释放按钮

2. **识别设备**：
   - Pico 将以 USB 大容量存储设备的形式出现在您的计算机上
   - 在 Windows 中会显示为 "RPI-RP2" 驱动器
   - 在 macOS 中会显示为 "RPI-RP2" 卷
   - 在 Linux 中会自动挂载（可以通过 `lsblk` 命令查看）

3. **传输固件**：
   - 将编译生成的 `.uf2` 文件复制到此存储设备
   - 文件复制完成后，Pico 将自动重启并运行新程序
   - 上传过程中设备上的 LED 会闪烁，上传完成后可能会持续亮起或按新程序的逻辑运行

### 方法二：使用 picotool 工具

对于需要自动化或更多控制的情况，可以使用 picotool：

1. **安装 picotool**：
   ```bash
   # 在 Linux 上安装
   sudo apt install picotool

   # 或从源码编译
   git clone https://github.com/raspberrypi/picotool
   cd picotool
   mkdir build
   cd build
   cmake ..
   make
   ```

2. **使用 picotool 上传固件**：
   ```bash
   # 确保 Pico 处于 BOOTSEL 模式
   picotool load -x path/to/your_firmware.uf2
   ```

### 方法三：使用 SWD 调试器上传

对于开发场景，可以使用 SWD 调试器：

1. **连接调试器**：
   - 将 SWD 调试器（如 Raspberry Pi Debug Probe 或 J-Link）连接到 Pico 的 SWD 引脚
   - 将调试器的 SWCLK 连接到 Pico 的 SWCLK 引脚
   - 将调试器的 SWDIO 连接到 Pico 的 SWDIO 引脚
   - 将调试器的 GND 连接到 Pico 的 GND 引脚

2. **使用 OpenOCD 上传**：
   ```bash
   openocd -f interface/picoprobe.cfg -f target/rp2040.cfg -c "program your_firmware.elf verify reset exit"
   ```

## 验证上传

上传成功后，您可以通过以下方式验证：

1. **观察设备行为**：
   - 如果固件包含 LED 控制代码，LED 将按程序逻辑闪烁
   - 如果使用显示屏，显示屏将显示相应信息

2. **通过串口监视**：
   - 使用串口终端软件（如 PuTTY、Screen 或 Arduino IDE 的串口监视器）
   - 配置正确的串口（在设备管理器中查看）和波特率（通常为 115200）
   - 查看设备输出的调试信息

## 常见问题

### 无法识别设备

- 确保 USB 数据线正常工作且支持数据传输（有些线只能充电）
- 尝试不同的 USB 端口，优先使用直接连接到主板的 USB 端口
- 检查计算机是否正确识别了设备（在设备管理器中查看）

### 上传后设备无响应

- 检查固件是否正确编译（没有编译错误）
- 确保上传了正确的固件文件（`.uf2` 格式）
- 尝试按下 Pico 上的 RESET 按钮重启设备
- 如果有多个 `.uf2` 文件，尝试上传另一个示例程序

### 上传过程中断开连接

- 这可能是由于静电放电或不稳定的连接导致
- 按住 BOOTSEL 按钮重新进入上传模式
- 使用更短或更高质量的 USB 线
- 确保供电稳定

## 高级技巧

### 默认启动程序

如果您希望 Pico 在每次上电时自动运行特定程序：

1. 上传所需的固件如前文所述
2. 程序将被永久保存在 Pico 的闪存中
3. 每次接通电源时，Pico 都会运行这个程序，除非按住 BOOTSEL 进入上传模式

### 多个固件管理

对于开发多个不同功能的项目：

1. 为不同功能的固件使用清晰的命名规则
2. 维护一个固件版本日志，记录每个版本的功能和变化
3. 考虑在编译时添加版本信息，通过串口输出或显示在屏幕上

## 参考资料

- [Raspberry Pi Pico 官方文档](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html)
- [UF2 文件格式说明](https://github.com/microsoft/uf2)
- [Picotool GitHub 仓库](https://github.com/raspberrypi/picotool) 