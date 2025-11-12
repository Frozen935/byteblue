# ByteBlue Stack 本地构建指南（启用 mbed TLS）

本项目是从 Zephyr Bluetooth 子系统抽取的独立模块，使用 GNU Make 构建。当前默认启用 mbed TLS（CONFIG_BYTEBLUE_CRYPTO_USE_MBEDTLS=y），支持“系统优先、缺省回退至本地”的依赖处理，以保证在干净环境也可稳定编译 demo。

## 环境依赖
- gcc（建议）或 clang
- python3（用于从 Kconfig 生成配置头文件）
- pip（首次构建会自动安装 `kconfiglib`）
- git（用于在无系统依赖时拉取本地 mbed TLS 源码）
- Linux/Unix 环境（示例依赖 pthread、rt）

## 依赖处理策略（系统优先 + 本地回退）
当 `CONFIG_BYTEBLUE_CRYPTO_USE_MBEDTLS=y` 时：
- 优先通过 `pkg-config` 检测系统安装的 mbed TLS：
  - 首选 `mbedcrypto`，若不存在则尝试 `mbedtls`
  - 自动获取编译包含路径（CFLAGS）与链接库（LIBS）
- 若系统未提供（`pkg-config` 不存在或未找到相应包），构建流程会自动：
  - 拉取并固定到指定版本：`third_party/mbedtls`（默认 `v2.28.8` LTS）
  - 使用其 `library/Makefile` 生成 `libmbedcrypto.a`
  - 将 `third_party/mbedtls/include` 加入编译包含路径

说明：demo 至少链接 `libmbedcrypto`；若系统的 `pkg-config mbedtls` 返回额外库（如 `mbedx509`, `mbedtls`），也会一并链接，不影响功能与稳定性。

## 一键构建
在仓库根目录执行：

```
make clean; make all
```

将生成：
- `libbyteblue.a`
- `samples/demo/demo`

首次执行会自动安装 `kconfiglib` 并生成 `include/generated/autoconf.h`。当系统未安装 mbed TLS 时，会自动拉取并构建本地 `third_party/mbedtls`（固定版本）。

## 常用目标
- `make all`：构建所有内容（默认目标）
- `make demo`：仅构建示例程序
- `make clean`：清理构建产物（不清理 `third_party/mbedtls` 源码与库）
- `make menuconfig`：打开 Kconfig 菜单并更新 `.config`

## 交叉编译
可通过环境变量覆盖工具链：

```
CROSS_COMPILE=aarch64-linux-gnu- make
```

## 故障排查
- 无法从网络拉取 `third_party/mbedtls`：
  - 请确保当前环境可访问 GitHub，或改为安装系统包（如 `sudo apt install libmbedtls-dev`，并确保 `pkg-config mbedcrypto` 可用）后重试。
- `pkg-config` 不存在：
  - 请安装 `pkg-config`（如 `sudo apt install pkg-config`），或依赖本地回退（自动拉取源码）。
- 链接阶段报找不到 `mbedcrypto`：
  - 表示系统库不可用且回退未成功构建，请检查上两项。

## 备注
- 示例程序位于 `samples/demo/main.c`，包含最小化的蓝牙初始化调用。
- 若需要启用更多特性，请在 `make menuconfig` 中调整配置后重新构建。

## 在其他工程调用 Bluetooth 模块（可复用的 module.mk）

Bluetooth 模块已独立为 `bluetooth/module.mk` 与 `bluetooth/Makefile`，可被外部工程直接 include 使用，或单独在本仓库内构建独立静态库。

- 关键变量：
  - `BT_PLATFORM`：选择平台（支持 `linux`、`nuttx`，预留 `freertos`）。
    - `linux`：使用 `osdep/posix/os.c` 与 `drivers/hci_sock.c`
    - `nuttx`：使用 `drivers/h4.c`（暂时仍用 `osdep/posix/os.c` 以保证最小可用）
  - `BT_AUTOCONF_H`：自动配置头文件路径（默认 `$(BT_ROOT)/include/generated/autoconf.h`）。模块不负责生成该文件，需由上层工程使用 Kconfig 工具链生成。
  - `BT_SRCS` / `BT_CPPFLAGS`：模块暴露的源文件列表与编译包含路径，已内含 `-include $(BT_AUTOCONF_H)` 与 `-include shim/include/shim.h`。

### NuttX apps 目录示例

在你的 NuttX `apps/` 子工程的 Makefile 中：

```makefile
# 假设通过环境或上层 Makefile 传入 BT_ROOT 到 byteblue 仓库根
BT_ROOT ?= $(TOPDIR)/byteblue

# 选择平台为 nuttx（默认 linux）
BT_PLATFORM ?= nuttx

# 引入蓝牙模块定义（暴露 BT_SRCS/BT_CPPFLAGS 等）
include $(BT_ROOT)/bluetooth/module.mk

# 将蓝牙源码加入你的工程源文件列表（或自有库目标）
MYAPP_SRCS += $(BT_SRCS)

# 追加编译包含路径与自动配置头
CPPFLAGS += $(BT_CPPFLAGS)

# 若启用 mbed TLS（CONFIG_BYTEBLUE_CRYPTO_USE_MBEDTLS），需为编译阶段提供头文件路径：
# （建议统一在上层 apps 框架处理，或参考本仓库根 Makefile 的 pkg-config 检测逻辑）
```

说明：模块侧不生成 autoconf.h，请在 NuttX 的 Kconfig 步骤中生成 `.config` 与 `include/generated/autoconf.h`，并置于 `BT_AUTOCONF_H` 指向的位置。

### Linux 外部工程示例（独立构建或复用源码）

- 直接构建独立蓝牙静态库（在本仓库内）：
  - `make -C bluetooth clean && make -C bluetooth -j`
  - 产物：`byteblue/bluetooth/libbyteblue_bt.a`
  - 可在你的工程中链接该静态库，同时链接本仓库 `base/*` 源码或相应库（模块库仅包含 bluetooth/* + 平台驱动/OS 层源码，不包含 base/* 与 core/*）。

- 在你的工程中复用源码（不直接链接 `libbyteblue_bt.a`）：

```makefile
# 假设 BT_ROOT 指向本仓库根目录
include $(BT_ROOT)/bluetooth/module.mk

# 将蓝牙源码与编译选项加入你的工程
MY_SRCS += $(BT_SRCS)
MY_CPPFLAGS += $(BT_CPPFLAGS)

# ... 其他编译规则 ...
```

## 维护者提示
- 根 Makefile 仍负责运行 Kconfig 与生成 `include/generated/autoconf.h`，默认 `make` 目标保持不变：同时生成 `libbyteblue.a` 与 `samples/demo`。
- 模块 Makefile（`bluetooth/module.mk`）不触发 Kconfig，仅暴露源文件列表与包含路径，供外部工程或子模块复用。
