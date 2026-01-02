# Setup and Usage Instructions

Complete instructions for building, configuring, and using the ESPNow Perfectly Adequate Arcade Pedal.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Wiring Diagram](#wiring-diagram)
- [Setup Instructions](#setup-instructions)
- [Configuration Options](#configuration-options)
- [Usage](#usage)
- [Debug Monitor](#debug-monitor)
- [Troubleshooting](#troubleshooting)

## Prerequisites

### Hardware Requirements

This project is configured for specific boards. Other boards may work but will require code modifications:

**Transmitter** - [FireBeetle 2 ESP32-E](https://www.dfrobot.com/product-2195.html):
- Debug toggle button on pin 27 (onboard button)
- Pin configuration: PEDAL_1_PIN=13, PEDAL_2_PIN=14, DEBUG_BUTTON_PIN=27
- Other ESP32 boards require changing these pin definitions

**Receiver** - [ESP32-S3-DevKitC-1-N16R8](https://www.amazon.com/dp/B0CC8NYXRG):
- Blue LED (WS2812 on pin 48) for grace period status
- Any ESP32-S2/S3 board works for USB HID, but LED code may need adjustment
- LED code can be disabled if not using this specific board

**Debug Monitor** (optional):
- Any ESP32 board with Serial/USB support

## Prerequisites

### Windows Developer Mode (Required for Symlinks)

**⚠️ WINDOWS USERS ONLY**: This project uses symbolic links to share code between transmitter and receiver. On Windows, you must enable Developer Mode:

1. Open **Settings** → **Privacy & security** → **For developers**
2. Toggle **Developer Mode** to ON
3. Confirm the warning dialog if prompted

**Why is this needed?**
- The `DebugMonitor.*` files in `transmitter/shared/` and `receiver/shared/` are symbolic links pointing to `shared/DebugMonitor.*`
- Without Developer Mode, Git will convert symlinks to plain text files, causing compilation errors
- Linux and macOS users can skip this step (symlinks work by default)

**After cloning the repo**, verify symlinks are working:
```powershell
ls receiver/shared/DebugMonitor.h
# Should show: Mode: la--- (symlink)
```

If symlinks aren't working after enabling Developer Mode, re-clone with:
```bash
git clone -c core.symlinks=true <repo-url>
```

## Wiring Diagram

### ⚠️ CRITICAL SAFETY WARNING

**The original schematic shows TP4056 OUT+ connected to the 3V3 pin. This is INCORRECT and WILL damage your ESP32!**

The TP4056 OUT+ **MUST** connect to **VCC**, NOT 3V3!

### Transmitter Power Wiring (FireBeetle 2 ESP32-E)

```
TP4056 Charging Board          FireBeetle 2 ESP32-E
─────────────────────          ─────────────────────
OUT+ (red wire)    ──────────> VCC (or PH2.0 battery connector)
OUT- (black wire)  ──────────> GND
B+                 ──────────> 18650 Battery +
B-                 ──────────> 18650 Battery -
IN+                ──────────> Power Switch
IN-                ──────────> GND
```

**Why VCC and not 3V3?**
- **VCC** is the input voltage pin that accepts battery voltage (3.7-4.2V) or USB voltage (~4.7V)
- **3V3** is a regulated 3.3V **OUTPUT** pin - connecting 4-5V to it **WILL DAMAGE** the ESP32
- The FireBeetle's onboard regulator converts VCC to 3.3V internally

### Transmitter Signal Wiring

```
Left Pedal Switch (NO)  ───> GPIO 13
Left Pedal Switch (COM) ───> GND

Right Pedal Switch (NO)  ───> GPIO 14 (dual mode only)
Right Pedal Switch (COM) ───> GND

LED (anode)            ───> GPIO 2 (optional)
LED (cathode)          ───> GND
```

**Notes:**
- Switches should be **normally-open (NO)** type
- ESP32's internal pull-up resistors keep pins HIGH when switches are open
- Pins go LOW when switches are pressed
- The original schematic may show a "Mode Select Switch" on GPIO 26 - this is **not needed** (mode is configured in code)

**Pinout Reference**: See [FireBeetle 2 ESP32-E Pinout](docs/FireBeetle2_ESP32-E_Pinout.md) for complete pin details.

### Receiver Wiring

The receiver only needs a USB connection to the computer. No additional wiring required.

## Setup Instructions

### Step 1: Upload Code to FireBeetle 2 ESP32-E

**⚠️ IMPORTANT**: To upload sketches to the FireBeetle 2 ESP32-E, you must ground GPIO 0 (D5) during upload:

1. Connect GPIO 0 (D5) to GND using a jumper wire or button
2. Press and hold the **RESET** button
3. While holding RESET, release GPIO 0 from GND (if using a button, release the button)
4. Release RESET button
5. The board should now be in download mode
6. Upload your sketch from Arduino IDE
7. After upload completes, disconnect GPIO 0 from GND

**Alternative method**: Some FireBeetle boards have a BOOT button. If your board has one:
- Hold BOOT while pressing RESET
- Release RESET first
- Then release BOOT

### Step 2: Configure Transmitter

Open `transmitter/transmitter.ino` and set the pedal mode:

```cpp
#define PEDAL_MODE 1  // 0 = DUAL_PEDAL, 1 = SINGLE_PEDAL
```

**Key assignment** (automatic, based on pairing order):
- First paired transmitter: LEFT pedal (`'l'`)
- Second paired transmitter: RIGHT pedal (`'r'`)
- For DUAL mode transmitters: `'1'` maps to `'l'`, `'2'` maps to `'r'`

### Step 3: Configure Receiver

The receiver requires **no configuration** - it automatically discovers and pairs with transmitters!

**LED Status Indicator** (ESP32-S3-DevKitC-1-N16R8):
- **Blue LED**: Receiver is in grace period (first 30 seconds after boot)
- **LED Off**: Normal operation after grace period

### Step 4: Upload Code

1. Upload `receiver/receiver.ino` to your ESP32-S2/S3 receiver board
2. Upload `transmitter/transmitter.ino` to your ESP32 transmitter board(s)
3. No MAC address configuration needed - everything is automatic!

## Configuration Options

### Transmitter Settings

Located in `transmitter/transmitter.ino`:

```cpp
#define PEDAL_MODE 1                    // 0 = DUAL_PEDAL, 1 = SINGLE_PEDAL
#define INACTIVITY_TIMEOUT 120000       // 2 minutes (in milliseconds)
#define IDLE_DELAY_PAIRED_MIN 10        // 10ms delay for responsiveness
#define IDLE_DELAY_PAIRED_MAX 20        // 20ms delay when idle
#define IDLE_DELAY_UNPAIRED 200         // 200ms delay when not paired
#define DEBOUNCE_DELAY 20               // 20ms debounce delay
#define DEBUG_BUTTON_PIN 27             // FireBeetle button for debug toggle
```

**Debug Toggle**: Press the button on pin 27 to toggle debug output on/off. State persists across reboots and deep sleep.

### Receiver Settings

Located in `receiver/receiver.ino`:

```cpp
#define MAX_PEDAL_SLOTS 2               // Maximum pedal slots (2 singles or 1 dual)
#define BEACON_INTERVAL 2000            // Beacon interval during grace period (ms)
#define TRANSMITTER_TIMEOUT 30000       // Grace period duration (30 seconds)
```

**Key assignment** is automatic based on pairing order:
- First transmitter: LEFT pedal (`'l'`)
- Second transmitter: RIGHT pedal (`'r'`)

## Usage

### Initial Pairing

1. **Power on the receiver first**
   - Receiver starts broadcasting availability beacons
   - Blue LED indicates grace period (30 seconds)

2. **Power on the transmitter(s)**
   - Transmitters automatically discover the receiver from beacons
   - Only stores receiver MAC if slots are available

3. **Press a pedal**
   - First pedal press triggers pairing
   - Transmitter sends discovery request
   - Receiver responds to complete pairing
   - Transmitter broadcasts pairing to other receivers

### Normal Operation

- **Press pedals to type keys** - Keys stay pressed until pedal is released
- **Both pedals work simultaneously** (for dual pedal mode)
- **Automatic reconnection** - Transmitters reconnect after reboot or deep sleep
- **Instant wake** - Pedal press wakes from deep sleep and sends the key immediately

### Discovery and Pairing Process

**How it works:**

1. **Receiver broadcasts beacons** during grace period (30 seconds)
   - Announces MAC address and available slots
   - Sent every 2 seconds

2. **Transmitter learns receiver MAC** from beacon
   - Only stores MAC if receiver has free slots
   - Waits for pedal press to initiate pairing

3. **Pedal press triggers pairing**
   - Transmitter sends `MSG_DISCOVERY_REQ` to receiver
   - Includes pedal mode (single/dual) for slot calculation

4. **Receiver responds**
   - Sends `MSG_DISCOVERY_RESP` to complete pairing
   - Adds transmitter to known list
   - Saves pairing to persistent storage

5. **Transmitter broadcasts pairing**
   - Sends `MSG_TRANSMITTER_PAIRED` with receiver MAC
   - Other receivers remove this transmitter if they had it paired

### Automatic Reconnection

**On transmitter boot:**
1. Transmitter broadcasts `MSG_TRANSMITTER_ONLINE` with its MAC
2. Receiver recognizes known transmitter
3. Receiver immediately sends `MSG_ALIVE` message
4. Transmitter pairs automatically without pedal press

**On receiver boot:**
1. Receiver sends `MSG_ALIVE` to all known transmitters immediately
2. Transmitters respond with `MSG_ALIVE` acknowledgment
3. Connection is re-established

### Deep Sleep and Wake

**After 2 minutes of inactivity:**
- Transmitter logs: `Inactivity timeout - entering deep sleep`
- Enters deep sleep to conserve battery
- Wake trigger: `ext0` on PEDAL_1_PIN (LOW)

**When pedal is pressed:**
1. Device wakes from deep sleep
2. Detects wake reason: `ESP_SLEEP_WAKEUP_EXT0`
3. Logs: `Woke from deep sleep - pedal pressed`
4. Immediately sends press event (if paired)
5. Waits for release (up to 5 seconds)
6. Sends release event
7. Logs: `Pedal released after wake`
8. Resets inactivity timer

**Result**: No missed key presses, even from deep sleep!

### Grace Period Details

**First 30 seconds after receiver boot:**
- Broadcasts availability beacons every 2 seconds
- Pings known transmitters that haven't been seen yet
- Blue LED indicator (ESP32-S3-DevKitC-1)
- Accepts new transmitter pairings (if slots available)

**After grace period:**
- Stops broadcasting beacons
- Only accepts reconnection from known transmitters
- LED turns off

## Debug Monitor

Since the receiver uses USB HID Keyboard, Serial output is not available. Use the wireless debug monitor instead.

### Setup Debug Monitor

1. **Upload code** to a second ESP32 board:
   - Use `debug-monitor/debug-monitor.ino`
   - Any ESP32 board with Serial/USB support works

2. **Power on the debug monitor**
   - Broadcasts `MSG_DEBUG_MONITOR_BEACON` every 5 seconds
   - Devices automatically pair when they see the beacon

3. **Open Serial Monitor** at 115200 baud
   - See debug messages from both receiver and transmitter
   - Messages prefixed with `[R]` (receiver) or `[T]` (transmitter)
   - Includes sender's MAC address

### How It Works

- **Debug monitor broadcasts beacon** with its MAC address
- **Devices pair automatically** when they receive the beacon
- **All debug messages** are sent via ESP-NOW to the monitor
- **Persistent pairing** - devices remember monitor across reboots
- **No timeout** - monitor always accepts new connections

### Debug Toggle (Transmitter)

- **Button**: Pin 27 (FireBeetle button)
- **Press to toggle**: Debug output on/off
- **State persists**: Across reboots and deep sleep
- **Stored in**: Preferences (EEPROM)

### Example Debug Output

```
[A0:85:E3:E0:8E:A8] [R] ESP-NOW initialized
[A0:85:E3:E0:8E:A8] [R] Loaded 1 transmitter(s) from storage
[A0:85:E3:E0:8E:A8] [R] Pedal slots used: 1/2
[A0:85:E3:E0:8E:A8] [R] === Receiver Ready ===
[A0:85:E3:E0:8E:A8] [R] Sending MSG_ALIVE to T0: 3C:8A:1F:B2:B7:3C
[3C:8A:1F:B2:B7:3C] [T] ESP-NOW initialized
[3C:8A:1F:B2:B7:3C] [T] === Transmitter Ready ===
[3C:8A:1F:B2:B7:3C] [T] Received MSG_ALIVE from paired receiver - replying
[3C:8A:1F:B2:B7:3C] [T] Pedal l PRESSED
[A0:85:E3:E0:8E:A8] [R] T0: 'l' ▼
[3C:8A:1F:B2:B7:3C] [T] Pedal l RELEASED
[A0:85:E3:E0:8E:A8] [R] T0: 'l' ▲
```

## Troubleshooting

### Transmitter not pairing

**Symptoms**: Transmitter doesn't pair when pedal is pressed

**Solutions**:
1. **Power on receiver first** - Receiver must be broadcasting beacons
2. **Check grace period** - Pairing only works during first 30 seconds after receiver boot (blue LED)
3. **Check receiver slots** - Receiver may be full (2 slots already used)
4. **Press pedal** - Transmitter only pairs when pedal is pressed
5. **Enable debug** - Press button on pin 27 to toggle debug, check debug monitor
6. **Check beacon reception** - Debug should show: `Beacon: slots=X/2`

### Keys not typing

**Symptoms**: Pedal press doesn't type keys on computer

**Solutions**:
1. **Check pairing status** - Transmitter must be paired with receiver
2. **Verify receiver is powered on** - Receiver must be running
3. **Check USB connection** - Receiver must be connected to computer via USB
4. **Check slot availability** - Receiver may have reached maximum slots (2)
5. **Verify ESP32 variant** - Both devices must support ESP-NOW
6. **Check debug output** - Should show: `T0: 'l' ▼` when pedal is pressed

### Transmitter not reconnecting after reboot

**Symptoms**: Transmitter doesn't reconnect automatically after power cycle

**Solutions**:
1. **Wait for receiver boot** - Receiver sends `MSG_ALIVE` on boot
2. **Check debug output** - Should show: `Received MSG_ALIVE from paired receiver`
3. **Verify transmitter online broadcast** - Debug should show: `Transmitter X came online`
4. **Check pairing persistence** - Receiver remembers paired transmitters across reboots
5. **Re-pair if needed** - Power on receiver, wait for grace period, press pedal

### Transmitter not waking from deep sleep

**Symptoms**: Pedal press doesn't wake transmitter from deep sleep

**Solutions**:
1. **Check wiring** - PEDAL_1_PIN (GPIO 13) must be connected correctly
2. **Verify switch type** - Must be normally-open (NO) switch
3. **Check debug output** - Should show: `Woke from deep sleep - pedal pressed`
4. **Test with shorter timeout** - Reduce `INACTIVITY_TIMEOUT` for testing
5. **Verify ext0 wakeup** - `esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, LOW)` is set

### Serial Monitor not working on receiver

**Symptoms**: No Serial output from receiver

**Expected behavior**: This is normal when USB HID Keyboard is active on ESP32-S2/S3

**Solutions**:
1. **Use debug monitor** - Upload `debug-monitor/debug-monitor.ino` to second ESP32
2. **Verify keyboard functionality** - Keyboard should still work even without Serial
3. **Check USB connection** - Receiver should appear as HID keyboard device
4. **Use debug messages** - All debug output goes to wireless debug monitor

### Multiple key presses (contact bounce)

**Symptoms**: Single pedal press registers as multiple key presses

**Solutions**:
1. **Adjust debounce delay** - Increase `DEBOUNCE_DELAY` in transmitter code
2. **Check switch quality** - Poor quality switches may have excessive bounce
3. **Add hardware debounce** - 0.1µF capacitor across switch contacts
4. **Check wiring** - Loose connections can cause intermittent contact
5. **Test different switches** - Some switches bounce more than others

### Symlink errors on Windows

**Symptoms**: Compilation errors about missing `DebugMonitor.h` or `DebugMonitor.cpp`

**Solutions**:
1. **Enable Developer Mode** - Settings → Privacy & security → For developers
2. **Re-clone repository** - `git clone -c core.symlinks=true <repo-url>`
3. **Verify symlinks** - `ls receiver/shared/DebugMonitor.h` should show `la---` mode
4. **Check Git config** - `git config core.symlinks` should return `true`
5. **Manual copy** (last resort) - Copy files from `shared/` to `receiver/shared/` and `transmitter/shared/`

### Debug monitor not receiving messages

**Symptoms**: Debug monitor shows no output or only partial output

**Solutions**:
1. **Check Serial Monitor baud rate** - Must be 115200
2. **Verify beacon broadcast** - Monitor broadcasts every 5 seconds
3. **Check device pairing** - Devices should auto-pair when they see beacon
4. **Power cycle devices** - Restart receiver and transmitter
5. **Check debug enabled** - Press button on pin 27 to toggle debug on transmitter
6. **Verify ESP-NOW initialization** - Debug should show: `ESP-NOW initialized`

### Receiver full, can't add new transmitter

**Symptoms**: New transmitter can't pair, receiver shows "full"

**Solutions**:
1. **Check current slots** - Debug shows: `Pedal slots used: X/2`
2. **Remove old transmitters** - Power on old transmitter, it will send `MSG_DELETE_RECORD`
3. **Wait for timeout** - Receiver pings old transmitters, removes non-responsive ones
4. **Manual reset** - Re-upload receiver code to clear all pairings
5. **Increase slots** - Change `MAX_PEDAL_SLOTS` in receiver code (not recommended)

## Additional Resources

- **[README.md](README.md)** - Project overview and features
- **[PERFORMANCE_OPTIMIZATIONS.md](PERFORMANCE_OPTIMIZATIONS.md)** - Performance tuning details
- **[FireBeetle Pinout](docs/FireBeetle2_ESP32-E_Pinout.md)** - Complete pin reference
- **[Original Design](https://www.printables.com/model/1220746-perfectly-adequate-arcade-pedal-wireless)** - 3D printable pedal enclosure

