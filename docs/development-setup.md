# 开发环境设置

本文档将指导您设置LC76G_Pico项目的开发环境。请按照以下步骤操作，以确保您能够成功编译和开发LC76G_Pico固件和应用程序。

## 目录

- [前提条件](#前提条件)
- [安装开发工具](#安装开发工具)
  - [Windows](#windows)
  - [macOS](#macos)
  - [Linux](#linux)
- [获取源代码](#获取源代码)
- [设置Raspberry Pi Pico SDK](#设置raspberry-pi-pico-sdk)
- [配置开发IDE](#配置开发ide)
  - [Visual Studio Code](#visual-studio-code)
  - [CLion](#clion)
- [验证环境](#验证环境)
- [常见问题](#常见问题)

## 前提条件

在开始之前，请确保您的系统满足以下要求：

- 具有至少4GB RAM的计算机
- 至少5GB的空闲磁盘空间
- 支持的操作系统：
  - Windows 10或以上
  - macOS 10.14或以上
  - Ubuntu 18.04或其他最新的Linux发行版

## 安装开发工具

### Windows

1. **安装编译工具链**

   安装ARM GCC工具链和构建工具：

   a. 安装MSYS2：
      - 访问 [https://www.msys2.org/](https://www.msys2.org/) 下载并安装MSYS2
      - 运行MSYS2 MinGW 64-bit终端
      - 执行以下命令更新包数据库：
        ```bash
        pacman -Syu
        ```
      - 安装必要的工具包：
        ```bash
        pacman -S --needed base-devel mingw-w64-x86_64-toolchain git mingw-w64-x86_64-cmake
        ```

   b. 安装ARM工具链：
      - 访问 [https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
      - 下载最新的ARM工具链（例如"gcc-arm-none-eabi-10.3-2021.10-win32.exe"）
      - 运行安装程序并按提示完成安装
      - 确保将安装目录添加到系统PATH环境变量

2. **安装Python 3**

   a. 访问 [https://www.python.org/downloads/windows/](https://www.python.org/downloads/windows/) 下载最新的Python 3安装程序
   b. 运行安装程序，选择"Add Python to PATH"
   c. 完成安装后，打开命令提示符验证安装：
      ```cmd
      python --version
      ```

3. **安装CMake**

   a. 访问 [https://cmake.org/download/](https://cmake.org/download/) 下载最新的Windows安装程序
   b. 运行安装程序，选择"Add CMake to the system PATH"
   c. 完成安装后，打开命令提示符验证安装：
      ```cmd
      cmake --version
      ```

4. **安装USB驱动**

   a. 安装Windows USB驱动以便与Raspberry Pi Pico通信：
      - 访问 [https://zadig.akeo.ie/](https://zadig.akeo.ie/) 下载Zadig
      - 连接Pico（按住BOOTSEL按钮接入USB）
      - 运行Zadig，选择Pico设备，并安装WinUSB驱动

### macOS

1. **安装Homebrew**

   如果尚未安装Homebrew，请打开终端并运行：
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

2. **安装必要工具**

   ```bash
   brew install cmake python3 git
   ```

3. **安装ARM工具链**

   ```bash
   brew install --cask gcc-arm-embedded
   ```

4. **验证安装**

   ```bash
   arm-none-eabi-gcc --version
   cmake --version
   python3 --version
   ```

### Linux (Ubuntu为例)

1. **安装基本开发工具**

   ```bash
   sudo apt update
   sudo apt install git cmake build-essential python3 python3-pip
   ```

2. **安装ARM工具链**

   ```bash
   sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
   ```

3. **安装其他依赖项**

   ```bash
   sudo apt install pkg-config libusb-1.0-0-dev
   ```

4. **验证安装**

   ```bash
   arm-none-eabi-gcc --version
   cmake --version
   python3 --version
   ```

## 获取源代码

1. **克隆LC76G_Pico代码库**

   打开终端或命令提示符，执行以下命令：

   ```bash
   git clone https://github.com/musicaJack/LC76G_Pico.git
   cd LC76G_Pico
   ```

2. **初始化并更新子模块**

   如果项目使用Git子模块，请执行：

   ```bash
   git submodule update --init --recursive
   ```

## 设置Raspberry Pi Pico SDK

LC76G_Pico基于Raspberry Pi Pico SDK开发。请按照以下步骤设置SDK：

1. **克隆Pico SDK仓库**

   在适当的目录中克隆Pico SDK（建议在LC76G_Pico代码库之外）：

   ```bash
   cd ..
   git clone https://github.com/raspberrypi/pico-sdk.git
   cd pico-sdk
   git submodule update --init
   ```

2. **设置环境变量**

   设置PICO_SDK_PATH环境变量，指向Pico SDK的绝对路径：

   - **Windows (PowerShell)**:
     ```powershell
     $env:PICO_SDK_PATH="C:\path\to\pico-sdk"
     ```

   - **Windows (命令提示符)**:
     ```cmd
     set PICO_SDK_PATH=C:\path\to\pico-sdk
     ```

   - **macOS/Linux**:
     ```bash
     export PICO_SDK_PATH=/path/to/pico-sdk
     ```

   为了使环境变量永久生效，建议将其添加到系统环境变量或启动脚本中。

   - **Windows**: 添加到系统环境变量（通过系统属性 -> 高级 -> 环境变量）
   - **macOS**: 添加到 `~/.zshrc` 或 `~/.bash_profile`
   - **Linux**: 添加到 `~/.bashrc` 或 `~/.profile`

## 配置开发IDE

### Visual Studio Code

1. **安装Visual Studio Code**

   从 [https://code.visualstudio.com/](https://code.visualstudio.com/) 下载并安装最新版本。

2. **安装必要的扩展**

   打开VS Code，转到扩展视图（快捷键：Ctrl+Shift+X），并安装以下扩展：

   - C/C++ Extension Pack (Microsoft)
   - CMake Tools (Microsoft)
   - Cortex-Debug (marus25)
   - Python (Microsoft)

3. **配置CMake**

   a. 打开LC76G_Pico文件夹：
      ```bash
      code path/to/LC76G_Pico
      ```

   b. 打开命令面板（快捷键：Ctrl+Shift+P），输入"CMake: Configure"并执行
   
   c. 选择"GCC ARM"作为编译器

   d. 在底部状态栏中，确保Build变体设置为"Debug"（开发期间）或"Release"（发布时）

4. **配置调试**

   a. 打开Run视图（快捷键：Ctrl+Shift+D）
   
   b. 点击"创建launch.json文件"选择"Cortex Debug"
   
   c. 配置launch.json如下（根据您的实际路径进行调整）：

   ```json
   {
       "version": "0.2.0",
       "configurations": [
           {
               "name": "Pico Debug",
               "cwd": "${workspaceRoot}",
               "executable": "${workspaceRoot}/build/src/lc76g_pico.elf",
               "request": "launch",
               "type": "cortex-debug",
               "servertype": "openocd",
               "device": "RP2040",
               "configFiles": [
                   "interface/picoprobe.cfg",
                   "target/rp2040.cfg"
               ],
               "svdFile": "${env:PICO_SDK_PATH}/src/rp2040/hardware_regs/rp2040.svd",
               "runToEntryPoint": "main",
               "postRestartCommands": [
                   "break main",
                   "continue"
               ]
           }
       ]
   }
   ```

### CLion

1. **安装CLion**

   从 [https://www.jetbrains.com/clion/](https://www.jetbrains.com/clion/) 下载并安装最新版本。

2. **配置工具链**

   a. 打开CLion，转到 File -> Settings -> Build, Execution, Deployment -> Toolchains
   
   b. 添加新的工具链，选择"Embedded GCC"类型
   
   c. 设置ARM工具链的路径：
      - Windows: 通常在 `C:\Program Files (x86)\GNU Arm Embedded Toolchain\...`
      - macOS: 通常在 `/usr/local/bin` 或 Homebrew安装路径
      - Linux: 通常在 `/usr/bin`

3. **打开项目**

   a. 选择 File -> Open，导航到LC76G_Pico目录
   
   b. 在弹出的对话框中，选择"Open as Project"
   
   c. 在CMake设置中，添加以下CMake参数：
      ```
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DPICO_SDK_PATH=路径/到/pico-sdk
      ```

4. **配置调试**

   a. 转到 Run -> Edit Configurations...
   
   b. 添加新的OpenOCD下载与运行配置
   
   c. 配置Board Config文件：
      ```
      -f interface/picoprobe.cfg -f target/rp2040.cfg
      ```
   
   d. 设置可执行文件为 `build/src/lc76g_pico.elf`

## 验证环境

完成上述设置后，执行以下步骤验证环境是否正确配置：

1. **构建示例项目**

   在终端或IDE中执行构建：

   ```bash
   cd LC76G_Pico
   mkdir build
   cd build
   cmake ..
   make -j4
   ```

   如果构建成功，将在build目录中生成二进制文件。

2. **刷写固件**

   按住Raspberry Pi Pico的BOOTSEL按钮并插入USB，然后释放按钮。Pico会以USB大容量存储设备的形式连接到计算机。

   复制生成的`.uf2`文件到Pico的虚拟U盘：

   ```bash
   cp build/src/lc76g_pico.uf2 /path/to/pico-drive/
   ```

   文件复制完成后，Pico将自动重启并运行新固件。

## 常见问题

### Q1: 找不到arm-none-eabi-gcc命令

确保ARM工具链已正确安装，并且其bin目录已添加到系统PATH环境变量中。您可以尝试重新安装工具链或手动添加其路径到PATH。

### Q2: 无法找到Pico SDK

确保您已设置PICO_SDK_PATH环境变量，并指向正确的Pico SDK路径。验证SDK已克隆并初始化了子模块。

### Q3: 编译时出现缺少头文件错误

这通常表示缺少一些依赖项。确保您已按照上述说明安装了所有必要的依赖项。对于不同的操作系统，可能需要安装特定的开发库。

### Q4: 无法连接到Pico进行调试

检查以下几点：
- 确保您使用的是适当的USB电缆（数据线而非仅充电线）
- 如果使用调试器，确认调试探针已正确连接
- 检查操作系统是否正确识别了设备
- Windows用户需确保已安装正确的USB驱动

### Q5: CMake错误：找不到编译器C标识

这通常是由于CMake无法找到正确的编译器。确保ARM工具链已安装并添加到PATH中。尝试在CMake命令中显式指定编译器：

```bash
cmake .. -DCMAKE_C_COMPILER=arm-none-eabi-gcc -DCMAKE_CXX_COMPILER=arm-none-eabi-g++
```

---

如果您遇到未在此处列出的问题，请参考[故障排除指南](troubleshooting.md)或在GitHub项目中提交issue。 