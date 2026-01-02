# ESPNow Perfectly Adequate Arcade Pedal

A wireless pedal system using ESP-NOW for low-latency communication between pedal transmitters and a USB HID keyboard receiver. Perfectly adequate for arcade gaming!

**Original Design**: Based on the [Perfectly Adequate Arcade Pedal (Wireless)](https://www.printables.com/model/1220746-perfectly-adequate-arcade-pedal-wireless) design on Printables.

## Features

- **Low latency**: ESP-NOW provides fast, direct communication without WiFi connection
- **Multiple pedal modes**: Single pedal (LEFT or RIGHT) or dual pedal configurations
- **Press and hold**: Keys stay pressed until pedal is released
- **Independent operation**: Both pedals can be pressed simultaneously
- **Battery efficient**: Inactivity timeout (2 minutes) and deep sleep support
- **Automatic discovery**: No manual MAC address configuration needed
- **Automatic reconnection**: Transmitters automatically reconnect after reboot or deep sleep
- **Instant wake**: Pedal press wakes from deep sleep and immediately sends the key
- **Slot management**: Receiver tracks available slots (max 2 pedal slots)
- **Grace period**: 30-second discovery window after receiver boot
- **Debug monitor**: Wireless ESP-NOW debug output to separate monitor device

## Hardware

### Required/Tested Boards

**Transmitter:**
- [FireBeetle 2 ESP32-E](https://www.dfrobot.com/product-2195.html) - **Required for current pin configuration**
  - Debug toggle button on pin 27 (onboard button)
  - Optimized for low power consumption
  - Other ESP32 boards will require pin configuration changes

**Receiver:**
- [ESP32-S3-DevKitC-1-N16R8](https://www.amazon.com/dp/B0CC8NYXRG) - **Required for LED status indicator**
  - Blue LED (WS2812 on pin 48) for grace period indication
  - Native USB HID support (any ESP32-S2/S3 works for HID)
  - Other ESP32-S2/S3 boards will work but may need LED code changes

**Debug Monitor (optional):**
- Any ESP32 board with Serial/USB support

### Components

**Transmitter:**
- ESP32 board (FireBeetle 2 ESP32-E recommended)
- Pedal switches (normally-open, NO type)
- TP4056 charging board
- 18650 battery
- Power switch
- Optional: LED for status indication

**Receiver:**
- ESP32-S2 or ESP32-S3 board (for USB HID support)
- USB connection to computer

## Quick Start

1. **Review safety warning**: [INSTRUCTIONS.md](INSTRUCTIONS.md#wiring-diagram) - The original schematic has incorrect power wiring that can damage your ESP32!
2. **Enable Developer Mode** (Windows only): Required for symlinks - see [INSTRUCTIONS.md](INSTRUCTIONS.md#windows-developer-mode)
3. **Configure pedal mode** in `transmitter/transmitter.ino`
4. **Upload code** to receiver and transmitter boards
5. **Pair**: Power on receiver, then transmitter, press a pedal

📖 **[Full setup instructions →](INSTRUCTIONS.md)**

## Project Structure

```
├── transmitter/          # Pedal transmitter code
│   ├── transmitter.ino
│   ├── domain/          # Business logic (PairingState, PedalReader)
│   ├── infrastructure/  # Hardware/network layer (EspNowTransport)
│   ├── application/     # Application services (PairingService, PedalService)
│   └── shared/          # Shared utilities (messages, debug monitor) [symlinks]
├── receiver/            # USB HID receiver code
│   ├── receiver.ino
│   ├── domain/          # Business logic (TransmitterManager)
│   ├── infrastructure/  # Hardware/network layer (EspNowTransport, Persistence, LEDService)
│   ├── application/     # Application services (PairingService, KeyboardService)
│   └── shared/          # Shared utilities (messages, debug monitor) [symlinks]
├── debug-monitor/       # Wireless debug monitor
│   └── debug-monitor.ino
└── shared/              # Shared code (source of truth)
    ├── messages.h       # ESP-NOW message structures
    ├── DebugMonitor.h   # Debug monitor interface
    └── DebugMonitor.cpp # Debug monitor implementation
```

**Architecture**: Clean Architecture principles with separation of concerns (Domain, Infrastructure, Application layers).

## How It Works

### Pairing Process

1. **Receiver broadcasts beacons** during 30-second grace period (includes MAC and available slots)
2. **Transmitter learns receiver MAC** from beacon (only if slots available)
3. **Pedal press triggers pairing** - transmitter sends discovery request
4. **Receiver responds** to complete pairing
5. **Transmitter broadcasts pairing** so other receivers know it's taken

### Reconnection

- **Transmitter boots**: Broadcasts `MSG_TRANSMITTER_ONLINE` with its MAC
- **Receiver recognizes**: Immediately sends `MSG_ALIVE` to known transmitters
- **Transmitter reconnects**: Pairs automatically without pedal press needed

### Deep Sleep

- **After 2 minutes of inactivity**: Transmitter enters deep sleep
- **Pedal press wakes**: Device wakes and immediately sends the key press
- **No missed inputs**: Press/release events are sent correctly even from sleep

## Debug Monitor

Since the receiver uses USB HID Keyboard, Serial output is not available. The project includes a wireless debug monitor that receives messages via ESP-NOW:

1. Upload `debug-monitor/debug-monitor.ino` to a second ESP32
2. Open Serial Monitor at 115200 baud
3. See debug messages from both receiver and transmitter wirelessly

Messages are prefixed with `[R]` (receiver) or `[T]` (transmitter) and include the sender's MAC address.

## Configuration

### Transmitter
- **Pedal mode**: `PEDAL_MODE` - 0 = DUAL_PEDAL, 1 = SINGLE_PEDAL
- **Inactivity timeout**: `INACTIVITY_TIMEOUT` - 2 minutes (120000ms)
- **Debug toggle**: Pin 27 button toggles debug output on/off

### Receiver
- **Max slots**: `MAX_PEDAL_SLOTS` - 2 slots (e.g., 2 singles or 1 dual)
- **Grace period**: `TRANSMITTER_TIMEOUT` - 30 seconds (30000ms)
- **Beacon interval**: `BEACON_INTERVAL` - 2 seconds during grace period

## Troubleshooting

See [INSTRUCTIONS.md](INSTRUCTIONS.md#troubleshooting) for detailed troubleshooting steps.

Common issues:
- **Not pairing**: Check grace period, receiver slots, enable debug
- **Keys not typing**: Verify pairing status, check receiver power
- **Serial not working on receiver**: Expected with HID - use debug monitor

## Performance

- **Low power**: 80MHz CPU, reduced WiFi power (10dBm), power save mode
- **Responsive**: 10-20ms loop delay when paired, dynamic based on activity
- **Fast pairing**: Instant reconnection via `MSG_ALIVE` on boot
- **Efficient**: ~300 lines of debug code shared via symlinks

## Documentation

- **[INSTRUCTIONS.md](INSTRUCTIONS.md)** - Detailed setup, wiring, and usage instructions
- **[PERFORMANCE_OPTIMIZATIONS.md](PERFORMANCE_OPTIMIZATIONS.md)** - Performance tuning details
- **[FireBeetle Pinout](docs/FireBeetle2_ESP32-E_Pinout.md)** - Pin reference for FireBeetle 2 ESP32-E

## License

This project is provided as-is for educational and personal use.

## Contributing

Feel free to submit issues or pull requests for improvements!
