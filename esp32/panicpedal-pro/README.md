# PanicPedal Pro Transmitter

This directory contains the firmware for the **PanicPedal Pro** transmitter PCB.

## Hardware

- **MCU**: ESP32-S3-WROOM
- **PCB**: Custom PanicPedal Pro PCB

## GPIO Pin Configuration

| GPIO | Function | Component |
|------|----------|-----------|
| GPIO2 | LED Control | Inolux_IN-PI554FCH LED |
| GPIO3 | Battery Voltage Sensing | Before TLV75733PDBV regulator |
| GPIO4 | Battery STAT1 | MCP73871 battery charger |
| GPIO5 | Switch Position 1 | 2-position switch (unused) |
| GPIO6 | Switch Position 2 | 2-position switch (unused) |
| GPIO7 | Foot Switch | Main pedal input |

## Configuration

- **Pedal Mode**: Single pedal (GPIO7)
- **Deep Sleep Wakeup**: GPIO7 (LOW trigger)

## Building and Uploading

1. Open `panicpedal-pro.ino` in Arduino IDE
2. Select your ESP32-S3 board from the board manager
3. Configure upload settings for your ESP32-S3-WROOM module
4. Upload the sketch

## Notes

- This code is specifically designed for the PanicPedal Pro PCB
- The foot switch is connected to GPIO7
- Battery monitoring pins are available but not yet implemented in the firmware
- LED control (GPIO2) is defined but not yet implemented
