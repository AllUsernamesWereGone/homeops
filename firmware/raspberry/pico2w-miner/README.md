# BitcoinMinerPico2W

Educational **Bitcoin lottery miner** project for the **Raspberry Pi Pico 2 W**, written in **C** using the **Raspberry Pi Pico SDK** and the **Raspberry Pi Pico VS Code Extension**.

The goal of this project is to understand Bitcoin block hashing, embedded C development, Wi-Fi networking on the Pico 2 W, ST7735 display output, and eventually the Stratum mining protocol.

> This project is educational. A Raspberry Pi Pico 2 W is far too slow to mine Bitcoin profitably.

---

## 1. Hardware Requirements

Required hardware:

```text
Raspberry Pi Pico 2 W
USB data cable
Computer with VS Code
2.4 GHz Wi-Fi network
```

Optional but currently used:

```text
Waveshare 1.8 inch ST7735 / ST7735S TFT display
Resolution: 128 x 160
```

Display wiring used in this project:

```c
#define PIN_SCK   10
#define PIN_MOSI  11
#define PIN_CS     9
#define PIN_DC     8
#define PIN_RST   12
#define PIN_BL    13
```

Display settings that are known to work:

```text
SPI port: spi1
Baudrate: 40 MHz
Width: 128
Height: 160
xstart: 2
ystart: 1
MADCTL: 0x00
Colors: correct
```

---

## 2. Software Prerequisites

Install the following on the development computer.

### 2.1 Visual Studio Code

Install **Visual Studio Code**.

### 2.2 Raspberry Pi Pico VS Code Extension

Install the official **Raspberry Pi Pico** extension in VS Code.

This project was built using the Pico extension generated CMake setup.

Known working versions / setup:

```text
Pico SDK: 2.2.0
Toolchain: 14_2_Rel1
Build system: CMake
IDE: Visual Studio Code
```

### 2.3 CMake Tools Extension

Install the **CMake Tools** extension if it is not already installed.

The Pico extension often installs or integrates this automatically, but if CMake commands are missing in VS Code, install it manually.

### 2.4 Git

Install Git if you want to clone the project from a repository.

Check Git with:

```bash
git --version
```

### 2.5 USB Serial Monitor

For reading debug output from the Pico, use one of the following:

```text
VS Code serial monitor
PuTTY
Tera Term
minicom
screen
```

Typical serial setting:

```text
Baud rate: 115200
```

For USB stdio, the exact baud rate usually does not matter, but `115200` is the common default.

---

## 3. Getting the Project

Clone the repository or copy the project folder.

Example:

```bash
git clone <repository-url>
cd BitcoinMinerPico2W
```

Expected project folder name:

```text
BitcoinMinerPico2W
```

Expected project target name:

```text
bitcoin_miner_pico2w
```

---

## 4. Important File Structure

Expected structure:

```text
BitcoinMinerPico2W/
├─ CMakeLists.txt
├─ pico_sdk_import.cmake
├─ lwipopts.h
├─ secrets.h              # local only, must not be committed
├─ secrets.example.h      # safe example file, can be committed
├─ README.md
└─ src/
   ├─ main.c
   ├─ miner.c
   ├─ miner.h
   ├─ sha256.c
   ├─ sha256.h
   ├─ bitcoin_job.c
   ├─ bitcoin_job.h
   ├─ bitcoin_target.c
   ├─ bitcoin_target.h
   ├─ display_st7735.c
   ├─ display_st7735.h
   ├─ display_ui.c
   ├─ display_ui.h
   ├─ wifi.c
   ├─ wifi.h
   ├─ stratum_tcp.c
   └─ stratum_tcp.h
```

Later planned files:

```text
src/stratum.c
src/stratum.h
```

---

## 5. Secrets Setup

The real Wi-Fi credentials and Bitcoin address are stored in:

```text
secrets.h
```

This file must **not** be committed to Git.

A safe template should exist:

```text
secrets.example.h
```

Create your real `secrets.h` by copying the example:

```bash
cp secrets.example.h secrets.h
```

On Windows Git Bash:

```bash
cp secrets.example.h secrets.h
```

Or manually copy `secrets.example.h` and rename it to `secrets.h`.

Example structure:

```c
#ifndef SECRETS_H
#define SECRETS_H

#define BTC_ADDRESS "bc1q-your-bitcoin-address-here"

#define STRATUM_HOST "eusolo.ckpool.org"
#define STRATUM_PORT 3333
#define WORKER_NAME "pico2w"
#define POOL_PASSWORD "x"

typedef struct {
    const char *ssid;
    const char *password;
} wifi_network_secret_t;

#define WIFI_NETWORK_COUNT 3

static const wifi_network_secret_t WIFI_NETWORKS[WIFI_NETWORK_COUNT] = {
    {"WIFI_NAME",  "WIFI_password"},
    {"WIFI_NAME2", "WIFI_password2"},
    {"WIFI_NAME3", "WIFI_password3"}
};

#endif
```

Use a **2.4 GHz Wi-Fi network**. The Pico 2 W does not use 5 GHz Wi-Fi.

---

## 6. Git Ignore Setup

Make sure `.gitignore` contains at least:

```gitignore
# Build output
build/
*.uf2
*.elf
*.bin
*.hex
*.map

# Local VS Code settings
.vscode/

# Secrets
secrets.h
src/secrets.h
```

Before committing, check that `secrets.h` is not staged:

```bash
git status
```

If `secrets.h` appears as staged or tracked, remove it from Git tracking:

```bash
git rm --cached secrets.h
```

Then commit the `.gitignore` change.

---

## 7. Opening the Project in VS Code

1. Open VS Code.
2. Open the folder `BitcoinMinerPico2W`.
3. Wait until the Pico extension and CMake extension detect the project.
4. Select the correct board if VS Code asks:

```text
Raspberry Pi Pico 2 W
```

5. Make sure the active CMake target is:

```text
bitcoin_miner_pico2w
```

---

## 8. Building the Project

### If `CMakeLists.txt` changed

Run:

```text
CMake: Configure
```

Then run:

```text
Raspberry Pi Pico: Compile Project
```

### If only `.c` or `.h` files changed

Usually only run:

```text
Raspberry Pi Pico: Compile Project
```

---

## 9. Flashing the Pico 2 W

There are two common ways to flash the Pico.

---

### Option A: Flash with the Pico VS Code Extension

Use the Raspberry Pi Pico extension command for flashing/running the project.

Depending on the extension setup, this may appear as one of these commands:

```text
Raspberry Pi Pico: Run Project
Raspberry Pi Pico: Flash Project
```

If this works, it is the easiest method.

---

### Option B: Flash manually using BOOTSEL

This method is reliable and works without debugger setup.

1. Unplug the Pico.
2. Hold the `BOOTSEL` button.
3. While holding `BOOTSEL`, plug the Pico into USB.
4. Release `BOOTSEL`.
5. A drive named something like this should appear:

```text
RPI-RP2
```

6. Build the project in VS Code.
7. Find the generated UF2 file, usually here:

```text
build/bitcoin_miner_pico2w.uf2
```

8. Copy the `.uf2` file onto the `RPI-RP2` drive.
9. The Pico automatically reboots and starts the program.

---

## 10. Running and Reading Serial Output

After flashing, open a serial monitor for the Pico USB serial port.

Typical settings:

```text
Baud rate: 115200
Line ending: any / default
```

Expected boot output contains messages similar to:

```text
BitcoinMinerPico2W boot test
SHA-256 self-test passed
Genesis header self-test passed
Bitcoin target self-test passed
WiFi: trying ...
WiFi: connected to ...
Pool: connecting to eusolo.ckpool.org:3333
DNS: resolving eusolo.ckpool.org
TCP: connected to pool
Pool: TCP connected
```

The display should also show miner/status information.

---

## 11. Current CMake Requirements

The project must include the project root so lwIP can find `lwipopts.h`:

```cmake
target_include_directories(bitcoin_miner_pico2w PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
```

The executable should include the source files:

```cmake
add_executable(bitcoin_miner_pico2w
    src/main.c
    src/miner.c
    src/sha256.c
    src/bitcoin_job.c
    src/bitcoin_target.c
    src/display_st7735.c
    src/display_ui.c
    src/wifi.c
    src/stratum_tcp.c
)
```

For the display SPI driver:

```cmake
target_link_libraries(bitcoin_miner_pico2w
    pico_stdlib
    hardware_spi
)
```

For Wi-Fi and lwIP networking:

```cmake
if (PICO_CYW43_SUPPORTED)
    target_link_libraries(bitcoin_miner_pico2w
        pico_cyw43_arch_lwip_threadsafe_background
    )
endif()
```

Earlier LED-only code may have used:

```cmake
pico_cyw43_arch_none
```

But real Wi-Fi networking requires:

```cmake
pico_cyw43_arch_lwip_threadsafe_background
```

---

## 12. `lwipopts.h`

The project needs `lwipopts.h` because lwIP requires a configuration file.

If this error appears:

```text
fatal error: lwipopts.h: No such file or directory
```

then either `lwipopts.h` is missing or the project root is not included in `target_include_directories`.

Current known working `lwipopts.h`:

```c
#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#define NO_SYS 1

#define LWIP_SOCKET 0
#define LWIP_NETCONN 0

#define LWIP_IPV4 1
#define LWIP_IPV6 0
#define LWIP_DHCP 1
#define LWIP_DNS 1
#define LWIP_ICMP 1
#define LWIP_RAW 1
#define LWIP_TCP 1
#define LWIP_UDP 1
#define LWIP_NETIF_HOSTNAME 1

#define MEM_LIBC_MALLOC 0
#define MEM_ALIGNMENT 4
#define MEM_SIZE 4000

#define MEMP_NUM_TCP_PCB 4
#define MEMP_NUM_TCP_SEG 16
#define MEMP_NUM_UDP_PCB 4
#define MEMP_NUM_SYS_TIMEOUT 8

#define PBUF_POOL_SIZE 24
#define PBUF_POOL_BUFSIZE 512

#define TCP_MSS 1460
#define TCP_SND_BUF (4 * TCP_MSS)
#define TCP_WND (4 * TCP_MSS)
#define TCP_SND_QUEUELEN ((4 * TCP_SND_BUF) / TCP_MSS)

#define LWIP_STATS 0
#define LWIP_PROVIDE_ERRNO 1

#endif
```

---

## 13. Wi-Fi Notes

Wi-Fi currently works with multiple configured networks from `secrets.h`.

Important settings that fixed earlier connection problems:

```c
#define WIFI_CONNECT_TIMEOUT_MS 30000
```

```c
cyw43_arch_init_with_country(CYW43_COUNTRY('A', 'T', 0))
```

```c
CYW43_AUTH_WPA2_MIXED_PSK
```

The country code currently used is Austria:

```c
CYW43_COUNTRY('A', 'T', 0)
```

Earlier failing output looked like:

```text
connect status: joining
connect status: no ip
WiFi: failed to connect ..., error -2
```

This was fixed by:

```text
increasing the timeout
using the Austria country code
using WPA2 mixed auth
```

---

## 14. Important LED Note

Before Wi-Fi was added, LED initialization used CYW43 initialization directly.

Now Wi-Fi owns CYW43 initialization.

Therefore, LED init should **not** call `cyw43_arch_init()` anymore.

For Pico W / Pico 2 W, LED init should return `PICO_OK` and let the Wi-Fi module initialize CYW43.

Example:

```c
int pico_led_init(void) {
#if defined(PICO_DEFAULT_LED_PIN)
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
#elif defined(CYW43_WL_GPIO_LED_PIN)
    return PICO_OK;
#endif
}
```

After Wi-Fi/CYW43 has been initialized, the LED can be toggled with:

```c
cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
```

---

## 15. Current Project Architecture

Keep modules separated by responsibility:

```text
wifi.c              -> Wi-Fi only
stratum_tcp.c       -> raw TCP only
stratum.c           -> Stratum JSON messages/parsing, later
bitcoin_job.c       -> block header / coinbase / merkle / job building
bitcoin_target.c    -> nBits and target comparison
miner.c             -> repeated hashing and share detection
display_ui.c        -> display output only
main.c              -> orchestration only
```

Do not mix Stratum parsing into the miner or display code.

---

## 16. What Already Works

Current confirmed working state:

```text
[✓] Pico 2 W C project builds
[✓] Project flashes to the Pico
[✓] USB serial monitor works
[✓] Onboard LED heartbeat works
[✓] Project renamed away from blink
[✓] Code moved into src/main.c
[✓] Dummy miner module created
[✓] Real SHA-256 implementation added
[✓] SHA-256 self-test passes
[✓] Double-SHA256 benchmark works
[✓] Local fake share detection works
[✓] Bitcoin genesis block header self-test passes
[✓] Bitcoin compact target / nBits conversion self-test works
[✓] ST7735 display works
[✓] Basic display text UI works
[✓] Wi-Fi connects using multiple configured networks
[✓] DNS resolution works
[✓] Raw TCP connection to Stratum pool works
```

Latest confirmed network stage:

```text
The Pico can connect to the Stratum pool with raw TCP.
```

---

## 17. What Still Needs To Be Done

Next development steps:

```text
1. Add TCP send function
2. Send mining.subscribe
3. Receive server response lines
4. Add small line buffer for newline-delimited JSON
5. Parse extranonce1 and extranonce2_size from subscribe response
6. Send mining.authorize
7. Parse mining.set_difficulty
8. Parse mining.notify
9. Build coinbase transaction: coinb1 + extranonce1 + extranonce2 + coinb2
10. double-SHA256 the coinbase transaction
11. Fold merkle branches
12. Build real 80-byte block header
13. Mine real pool job locally
14. Submit share using mining.submit
15. Add reconnect logic
```

First Stratum message to send later:

```json
{"id":1,"method":"mining.subscribe","params":["pico2w-miner/0.1"]}
```

Authorize message will likely use:

```text
BTC_ADDRESS.WORKER_NAME
```

with password:

```text
x
```

The exact pool-specific username format should be checked before finalizing the Stratum implementation.

---

## 18. Troubleshooting

### Pico does not appear as `RPI-RP2`

Try:

```text
1. Use a real USB data cable, not a charge-only cable.
2. Unplug the Pico.
3. Hold BOOTSEL.
4. Plug it in while still holding BOOTSEL.
5. Release BOOTSEL after plugging it in.
6. Try a different USB port.
```

---

### Build fails after changing `CMakeLists.txt`

Run:

```text
CMake: Configure
```

Then:

```text
Raspberry Pi Pico: Compile Project
```

---

### `secrets.h` not found

Create it from the example:

```bash
cp secrets.example.h secrets.h
```

Then fill in real Wi-Fi credentials and Bitcoin address.

---

### Wi-Fi does not connect

Check:

```text
1. SSID is correct.
2. Password is correct.
3. Network is 2.4 GHz.
4. Timeout is long enough.
5. Country code is set correctly.
6. Auth mode matches the router.
```

Useful known settings:

```c
#define WIFI_CONNECT_TIMEOUT_MS 30000
cyw43_arch_init_with_country(CYW43_COUNTRY('A', 'T', 0))
CYW43_AUTH_WPA2_MIXED_PSK
```

---

### Serial monitor shows nothing

Try:

```text
1. Close other serial monitors.
2. Replug the Pico.
3. Select the correct COM port.
4. Open serial at 115200 baud.
5. Press reset or reflash the Pico.
```

Also make sure the code calls:

```c
stdio_init_all();
```

A short delay after boot is useful:

```c
sleep_ms(2000);
```

---

### Display stays black

Check:

```text
1. Wiring pins
2. Backlight pin BL
3. SPI port
4. Display power
5. Display driver initialization
```

Known working display pins:

```c
#define PIN_SCK   10
#define PIN_MOSI  11
#define PIN_CS     9
#define PIN_DC     8
#define PIN_RST   12
#define PIN_BL    13
```

---

## 19. Recommended First Test on a New Setup

For a completely new computer or IDE setup, verify in this order:

```text
1. VS Code opens the project
2. Pico extension is installed
3. CMake configures successfully
4. Project compiles successfully
5. UF2 file is generated
6. Pico enters BOOTSEL mode
7. UF2 can be copied to RPI-RP2
8. Serial monitor shows boot output
9. SHA-256 self-test passes
10. Wi-Fi connects
11. TCP connection to pool succeeds
```

Do not continue with Stratum protocol work until all of these are working.

---

## 20. Current Development Rule

Keep each step small and testable.

Good next step:

```text
Send mining.subscribe and print the raw server response.
```

Bad next step:

```text
Implement all of Stratum, JSON parsing, job building, mining, and share submission at once.
```

The project should always remain in a state where it still builds, flashes, and prints useful debug output over USB serial.
