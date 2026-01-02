# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- RTC memory persistence for pairing state across deep sleep for instant resume
- Transmitter now restores pairing from RTC memory on wake from deep sleep
- No reconnection delay when waking from deep sleep - acts as if never disconnected
- Receiver sends `MSG_ALIVE` to all known transmitters immediately on boot
- Transmitter replies to `MSG_ALIVE` messages from paired receiver with acknowledgment
- Debug messages showing MAC addresses for `MSG_ALIVE` ping operations
- Deep sleep detection and immediate pedal handling on wake
- Debug message when entering deep sleep with RTC pairing status
- Comprehensive debug output for reconnection flow

### Changed
- Split documentation: README.md for project overview, INSTRUCTIONS.md for detailed setup
- Improved hardware requirements clarity: specified board-specific features vs compatibility
- Transmitter skips `MSG_TRANSMITTER_ONLINE` broadcast when restored from RTC memory
- Deep sleep now saves pairing to RTC memory for instant resume
- Enhanced deep sleep debug messages with pairing state information

### Fixed
- Transmitter now immediately handles pedal press that woke device from deep sleep
- No missed key presses when waking from deep sleep

## [1.0.0] - 2025-01-02

### Added
- Shared `DebugMonitor` implementation using symbolic links
- Transport abstraction layer using function pointers for code reuse
- Callback support for custom status messages in debug monitor
- Windows Developer Mode instructions for symlink support
- Runtime debug toggle using pin 27 button (FireBeetle)
- Debug state persistence across deep sleep and reboots
- Debug monitor beacon broadcasting (`MSG_DEBUG_MONITOR_BEACON`)
- Automatic debug monitor discovery and pairing
- `[T]` and `[R]` prefixes for transmitter and receiver debug messages
- MAC address display in debug monitor output from packet sender
- Concise pedal event debug format: `T0: 'l' ▼` / `T0: 'l' ▲`
- Performance optimizations: 80MHz CPU, reduced WiFi power (10dBm), dynamic loop delays
- Inactivity timeout reduced to 2 minutes for better battery life
- Power optimization with WiFi power save mode (`WIFI_PS_MAX_MODEM`)
- INSTRUCTIONS.md with complete setup and troubleshooting guide
- PERFORMANCE_OPTIMIZATIONS.md documenting optimization techniques

### Changed
- Refactored `DebugMonitor` from infrastructure to shared directory
- Reduced codebase by ~300 lines through shared code
- Debug monitor uses sender MAC from ESP-NOW packets instead of reading own MAC
- Improved debug message clarity and reduced spam
- Removed routine debug messages (ALIVE, successful sends, beacon details)
- Only log significant events (pairing, conflicts, errors, state changes)
- Dynamic loop delay: 10ms when active, 20ms when idle, 200ms when unpaired
- Improved README with better project presentation
- Updated hardware requirements to clarify board-specific features

### Removed
- Duplicate `DebugMonitor` files from transmitter/infrastructure and receiver/infrastructure
- Discovery timeout messages from transmitter
- Debug monitor auto-unpairing logic based on send failures
- Excessive debug output for routine operations
- Redundant status messages and timestamps from debug output

### Fixed
- Arduino IDE compilation with symlinks by copying to project directories
- Debug monitor message mangling by sending actual message length instead of full struct
- Extra newlines in debug output
- Symlink support on Windows requiring Developer Mode
- Duplicate helper functions between `DebugMonitor.cpp` and `messages.h`
- Serial buffer size increased to 2048 bytes for atomic printing

## [0.9.0] - 2025-01-01

### Added
- Clean Architecture refactoring with Domain, Infrastructure, and Application layers
- Automatic discovery and pairing system
- Beacon broadcasting during 30-second grace period
- `MSG_TRANSMITTER_ONLINE` broadcast for automatic reconnection
- `MSG_TRANSMITTER_PAIRED` broadcast to prevent duplicate pairings
- Slot management system (max 2 pedal slots)
- Persistent storage of transmitter pairings on receiver
- Automatic key assignment based on pairing order
- Deep sleep support with 10-minute inactivity timeout
- Debug monitor for wireless ESP-NOW debug output
- Multi-color LED status indicator on receiver (blue during grace period)
- Transmitter replacement mechanism when receiver is full

### Changed
- Restructured codebase following Clean Architecture principles
- Transmitters only store receiver MAC if slots are available
- Receiver remembers all known transmitters permanently
- Pairing only occurs during grace period for new transmitters
- Known transmitters can reconnect anytime
- Improved battery efficiency with optimized loop delays

### Fixed
- Receiver LED control using Adafruit NeoPixel library
- MAC address validation before ESP-NOW operations
- ESP-NOW initialization with retry mechanism
- Const correctness in function signatures

## [0.1.0] - 2024-12-15

### Added
- Initial ESP-NOW wireless pedal implementation
- Basic transmitter code for pedal input
- Basic receiver code with USB HID Keyboard output
- Single and dual pedal mode support
- Press and hold functionality
- Debouncing for pedal switches
- Manual MAC address configuration
- FireBeetle 2 ESP32-E support
- ESP32-S3-DevKitC-1 receiver support

### Changed
- Migrated from Bluetooth to ESP-NOW for lower latency

[Unreleased]: https://github.com/chdao/espnow-perfectly-adequate-arcade-pedal/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/chdao/espnow-perfectly-adequate-arcade-pedal/compare/v0.9.0...v1.0.0
[0.9.0]: https://github.com/chdao/espnow-perfectly-adequate-arcade-pedal/compare/v0.1.0...v0.9.0
[0.1.0]: https://github.com/chdao/espnow-perfectly-adequate-arcade-pedal/releases/tag/v0.1.0

