# NUCLEO-F767ZI + Zephyr

Board docs: <https://docs.zephyrproject.org/latest/boards/st/nucleo_f767zi/doc/index.html>

Setup Guide: <https://docs.zephyrproject.org/latest/develop/getting_started/index.html>

- **west**: 1.5.0
- **Zephyr SDK**: 0.17.4
- **Python**: 3.12

## Quick Installation

Install script using `curl`

### Linux and MacOS in terminal

```bash
curl -O https://raw.githubusercontent.com/UiASub/K2-Zephyr/main/install_zephyr.sh
chmod +x install_zephyr.sh
./install_zephyr.sh
```

use `./install_zephyr.sh -h` to see help

### Windows in cmd or PowerShell

```bash
curl -O https://raw.githubusercontent.com/UiASub/K2-Zephyr/main/install_zephyr.ps1
powershell -ExecutionPolicy Bypass -File install_zephyr.ps1
```

You need [winget](https://aka.ms/getwinget) to install dependencies.

## Manual Installation

- **west**: 1.5.0
- **Zephyr SDK**: 0.17.4
- **Python**: 3.12

### Dependencies

**Windows**: Use [winget](https://aka.ms/getwinget)
then run this in ps or cmd:

```bash
winget install Kitware.CMake Ninja-build.Ninja oss-winget.gperf Python.Python.3.12 Git.Git oss-winget.dtc wget 7zip.7zip STMicroelectronics.STM32CubeProgrammer
```

>You may need to add the 7zip and STM32CubeProgrammer installation folders to your PATH.

**Ubuntu**: Use apt:

```bash
sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget python3.12 python3.12-dev python3.12-venv python3-tk \
  xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1 openocd
```

> **Note**: Linux users can install STM32CubeProgrammer manually from [STMicroelectronics](https://www.st.com/en/development-tools/stm32cubeprog.html) for faster flashing. OpenOCD (installed above) works as an alternative.

verify:

```bash
cmake --version
python3 --version
dtc --version
```

**MacOS**: Use [Homebrew](https://brew.sh/)

```bash
brew install cmake ninja gperf python@3.12 ccache qemu dtc libmagic wget
brew install --cask stm32cubeprogrammer
```

### Get Zephyr and install Python dependencies

Using `pip` on *Linux* or *MacOS*:

```bash
python3.12 -m venv ~/zephyrproject/.venv
source ~/zephyrproject/.venv/bin/activate
pip install west==1.5.0
west init ~/zephyrproject
cd ~/zephyrproject
west update
west zephyr-export
west packages pip --install
cd ~/zephyrproject/zephyr
west sdk install --version 0.17.4
```

Using `uv`:

```bash
cd ~/zephyrproject
uv venv --python 3.12
source .venv/bin/activate
uv pip install west==1.5.0
west init ~/zephyrproject
west update
west zephyr-export
west packages pip --install
cd ~/zephyrproject/zephyr
west sdk install --version 0.17.4
```

in Powershell on *Windows*:

```bash
cd $Env:HOMEPATH
python -m venv zephyrproject\.venv
zephyrproject\.venv\Scripts\Activate.ps1
pip install west==1.5.0
west init zephyrproject
cd zephyrproject
west update
west zephyr-export
west packages pip --install
cd $Env:HOMEPATH\zephyrproject\zephyr
west sdk install --version 0.17.4
```

## Build & flash

Run `build.sh` or inside of `/K2-Zephyr` run:

```bash
west build -b nucleo_f767zi
west flash
```

> **Note**: `west flash` uses STM32CubeProgrammer by default (installed on Windows/macOS). Linux users can install it manually or use OpenOCD fallback.

To explicitly use OpenOCD:

```bash
west flash --runner openocd
```

Monitor serial (115200 baud):

```bash
# Linux
minicom -D /dev/ttyACM0 -b 115200

# macOS
minicom -D /dev/tty.usbmodem* -b 115200
# or use screen
screen /dev/tty.usbmodem* 115200
```

## vscode config

> This is so no errors appear in c library for zephyr

In `.vscode` folder, add this and customize to your need

`c_cpp_properties.json`

```json
{
    "configurations": [
        {
            "name": "Zephyr ARM",
            "includePath": [
                "${workspaceFolder}/build/zephyr/include/generated",
                "${workspaceFolder}/**",
                "~/zephyrproject/zephyr/include",
                "~/zephyrproject/zephyr/include/zephyr",
                "~/zephyrproject/zephyr/lib/libc/common/include",
                "~/zephyrproject/zephyr/lib/libc/minimal/include",
                "~/zephyr-sdk-VERSION/arm-zephyr-eabi/arm-zephyr-eabi/include",
                "~/zephyrproject/zephyr/soc/st/stm32/stm32f7x",
                "~/zephyrproject/zephyr/soc/st/stm32/common",
                "~/zephyrproject/zephyr/modules/cmsis",
                "~/zephyrproject/zephyr/modules/cmsis_6",
                "~/zephyrproject/modules/hal/stm32/stm32cube/stm32f7xx/soc",
                "~/zephyrproject/modules/hal/stm32/stm32cube/stm32f7xx/drivers/include",
                "~/zephyrproject/modules/hal/stm32/stm32cube/common_ll/include"
            ],
            "forcedInclude": [
                "${workspaceFolder}/build/zephyr/include/generated/zephyr/autoconf.h"
            ],
            "defines": [
                "CONFIG_BOARD_NUCLEO_F767ZI",
                "__ZEPHYR__=1",
                "CONFIG_ARM=1",
                "CONFIG_CPU_CORTEX_M7=1",
                "CONFIG_SOC_STM32F767XX=1",
                "CONFIG_SYS_CLOCK_TICKS_PER_SEC=10000"
            ],
            "compilerPath": "~/zephyr-sdk-VERSION/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc",
            "cStandard": "c11",
            "cppStandard": "c++17",
            "intelliSenseMode": "gcc-arm"
        }
    ],
    "version": 4
}
```
