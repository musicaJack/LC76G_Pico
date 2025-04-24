# 构建指南

本文档提供了编译和构建LC76G_Pico项目的详细步骤。按照本指南，您可以成功地构建固件并将其上传到Raspberry Pi Pico板上。

## 目录

- [前提条件](#前提条件)
- [获取源代码](#获取源代码)
- [项目结构](#项目结构)
- [构建配置](#构建配置)
  - [配置选项](#配置选项)
  - [环境变量](#环境变量)
- [构建步骤](#构建步骤)
  - [命令行构建](#命令行构建)
  - [使用IDE构建](#使用ide构建)
- [构建输出](#构建输出)
- [固件类型](#固件类型)
- [自定义构建](#自定义构建)
- [故障排除](#故障排除)

## 前提条件

在开始构建LC76G_Pico项目之前，请确保您已经：

1. 按照[开发环境设置](development-setup.md)文档安装了所有必要的工具和依赖
2. 正确设置了Raspberry Pi Pico SDK
3. 设置了`PICO_SDK_PATH`环境变量指向Pico SDK的路径

## 获取源代码

如果您尚未获取源代码，请执行以下步骤：

```bash
# 克隆代码仓库
git clone https://github.com/your-organization/LC76G_Pico.git
cd LC76G_Pico

# 初始化并更新子模块
git submodule update --init --recursive
```

## 项目结构

了解项目结构对于成功构建非常重要：

```
LC76G_Pico/
├── CMakeLists.txt         # 主CMake配置文件
├── src/                   # 源代码目录
│   ├── main.c             # 主程序入口
│   ├── lc76g/             # LC76G模块相关代码
│   ├── drivers/           # 硬件驱动
│   └── utils/             # 实用工具函数
├── include/               # 头文件目录
├── tests/                 # 测试代码
├── examples/              # 示例代码
└── build/                 # 构建输出目录（会在构建过程中创建）
```

## 构建配置

LC76G_Pico项目使用CMake构建系统，可以通过多种选项自定义构建过程。

### 配置选项

以下是可用的CMake配置选项：

| 选项                     | 说明                             | 默认值  |
|--------------------------|----------------------------------|---------|
| `BUILD_EXAMPLES`         | 是否构建示例代码                 | ON      |
| `BUILD_TESTS`            | 是否构建测试代码                 | OFF     |
| `ENABLE_DEBUG_OUTPUT`    | 启用调试输出                     | ON      |
| `USE_I2C_INTERFACE`      | 使用I2C接口与LC76G模块通信       | ON      |
| `USE_UART_INTERFACE`     | 使用UART接口与LC76G模块通信      | OFF     |
| `PICO_BOARD`             | 目标Pico板类型                   | pico    |

可以通过以下方式设置这些选项：

```bash
cmake -DBUILD_EXAMPLES=OFF -DUSE_UART_INTERFACE=ON ..
```

### 环境变量

构建系统使用以下环境变量：

- `PICO_SDK_PATH`: Raspberry Pi Pico SDK的路径
- `PICO_TOOLCHAIN_PATH`: ARM交叉编译工具链的路径（可选）

## 构建步骤

### 命令行构建

按照以下步骤从命令行构建项目：

1. 创建并进入构建目录：

   ```bash
   mkdir -p build
   cd build
   ```

2. 配置项目：

   ```bash
   cmake ..
   ```

   或者使用自定义配置：

   ```bash
   cmake -DBUILD_EXAMPLES=ON -DENABLE_DEBUG_OUTPUT=ON ..
   ```

3. 编译项目：

   ```bash
   make -j4   # 使用4个并行作业加速构建
   ```

   或者在Windows上使用：

   ```bash
   cmake --build . --config Release
   ```

### 使用IDE构建

#### Visual Studio Code

1. 打开项目文件夹
2. 按`F1`并输入`CMake: Configure`
3. 选择`GCC ARM`作为编译器
4. 按`F1`并输入`CMake: Build`，或点击底部状态栏中的`Build`按钮

#### CLion

1. 打开项目文件夹
2. 确保正确配置了工具链（File > Settings > Build, Execution, Deployment > Toolchains）
3. 点击`Build`按钮或使用快捷键`Ctrl+F9`

## 构建输出

成功构建后，您将在`build`目录中找到以下输出文件：

- `src/lc76g_pico.elf`: ELF格式的固件文件（用于调试）
- `src/lc76g_pico.bin`: 二进制格式的固件
- `src/lc76g_pico.uf2`: UF2格式的固件（用于通过USB大容量存储模式上传）
- `examples/`: 示例程序的编译输出（如果启用）
- `tests/`: 测试程序的编译输出（如果启用）

## 固件类型

LC76G_Pico项目提供几种不同的固件类型：

1. **标准固件** - 包含所有功能的完整固件
2. **精简固件** - 移除了一些高级功能以减小固件大小
3. **调试固件** - 包含额外的调试功能和日志记录

要构建特定类型的固件，请设置`FIRMWARE_TYPE`变量：

```bash
cmake -DFIRMWARE_TYPE=standard ..  # 标准固件（默认）
cmake -DFIRMWARE_TYPE=minimal ..   # 精简固件
cmake -DFIRMWARE_TYPE=debug ..     # 调试固件
```

## 自定义构建

### 添加自定义功能

如果您想在构建中添加自定义功能，可以编辑`CMakeLists.txt`文件或创建自己的CMake配置文件。

例如，添加新的源文件：

```cmake
# 在您的CMakeLists.txt中
target_sources(lc76g_pico PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/your_custom_file.c
)
```

### 修改默认配置

您可以通过编辑`include/lc76g_config.h`文件修改默认配置：

```c
// 修改这些值以自定义行为
#define LC76G_UART_BAUD_RATE 9600
#define LC76G_I2C_ADDRESS 0x42
#define LC76G_UPDATE_INTERVAL_MS 1000
```

## 故障排除

### 常见构建错误

#### CMake错误：无法找到Pico SDK

确保`PICO_SDK_PATH`环境变量已正确设置，或者在运行CMake时明确指定：

```bash
cmake -DPICO_SDK_PATH=/path/to/pico-sdk ..
```

#### 编译错误：无法找到头文件

确保已正确初始化子模块：

```bash
git submodule update --init --recursive
```

#### 链接错误

确保使用正确的ARM工具链版本，并且所有依赖项都已安装。

### 构建优化

如果构建过程太慢，尝试：

- 使用并行构建：`make -j$(nproc)`
- 使用Ninja代替Make：`cmake -G Ninja .. && ninja`
- 减少构建类型：`cmake -DCMAKE_BUILD_TYPE=MinSizeRel ..`

---

如果您在构建过程中遇到未在此列出的问题，请参考[故障排除](troubleshooting.md)文档或在GitHub项目中提交issue。 