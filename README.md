# OpenBlue Stack Local Build Guide (with mbed TLS Enabled)

This project is an independent module extracted from the Zephyr Bluetooth subsystem, built using GNU Make. It currently enables mbed TLS by default (CONFIG_OPENBLUE_CRYPTO_USE_MBEDTLS=y) and supports "system-first, fallback-to-local" dependency handling to ensure stable compilation of demos even in clean environments.

## Environment Dependencies
- gcc (recommended) or clang
- python3 (for generating configuration header files from Kconfig)
- pip (kconfiglib will be automatically installed on first build)
- git (used to fetch local mbed TLS source code when system dependencies are not available)
- Linux/Unix environment (examples depend on pthread, rt)

## Dependency Handling Strategy (System-First + Local Fallback)
When `CONFIG_OPENBLUE_CRYPTO_USE_MBEDTLS=y`:
- Prioritize detecting system-installed mbed TLS via `pkg-config`:
  - Prefer `mbedcrypto`, fallback to `mbedtls` if not found
  - Automatically obtain compilation include paths (CFLAGS) and link libraries (LIBS)
- If not provided by system (`pkg-config` not found or package not found), the build process will automatically:
  - Fetch and fix to specified version: `third_party/mbedtls` (default `v2.28.8` LTS)
  - Use its `library/Makefile` to generate `libmbedcrypto.a`
  - Add `third_party/mbedtls/include` to compilation include paths

Note: The demo links at least `libmbedcrypto`; if the system's `pkg-config mbedtls` returns additional libraries (such as `mbedx509`, `mbedtls`), they will also be linked without affecting functionality and stability.

## One-Click Build
Execute in the repository root directory:

```
make clean; make all
```

This will generate:
- `libopenblue.a`
- `samples/demo/demo`

The first execution will automatically install `kconfiglib` and generate `include/generated/autoconf.h`. When mbed TLS is not installed in the system, it will automatically fetch and build the local `third_party/mbedtls` (fixed version).

## Common Targets
- `make all`: Build everything (default target)
- `make demo`: Build only the sample program
- `make clean`: Clean build artifacts (does not clean `third_party/mbedtls` source code and libraries)
- `make menuconfig`: Open Kconfig menu and update `.config`

## Cross Compilation
Toolchain can be overridden via environment variables:
`CROSS_COMPILE=aarch64-linux-gnu- make`


## Troubleshooting
- Unable to fetch `third_party/mbedtls` from network:
  - Ensure current environment can access GitHub, or install system packages instead (e.g., `sudo apt install libmbedtls-dev`, and ensure `pkg-config mbedcrypto` is available) then retry.
- `pkg-config` not found:
  - Please install `pkg-config` (e.g., `sudo apt install pkg-config`), or rely on local fallback (automatically fetch source code).
- Linking stage reports `mbedcrypto` not found:
  - Indicates system library is unavailable and fallback build was not successful, please check the above two items.

## Notes
- Sample program is located at `samples/demo/main.c`, containing minimal Bluetooth initialization calls.
- If more features are needed, please adjust configuration in `make menuconfig` and rebuild.
