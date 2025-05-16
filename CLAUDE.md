# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Hardware Overview

The TournaFence hardware component consists of two microcontrollers that work together to create a wireless fencing scoring system:
- **ESP32**: Handles BLE communication with the mobile app, acting as a bridge
- **Arduino Mega**: Implements core fencing logic, timing, and physical sensor detection

## Communication Architecture

```
Mobile App <--(BLE)--> ESP32 <--(UART Serial1/Serial2)--> Arduino Mega
```

### BLE Protocol (ESP32 <--> Mobile App)
- Service UUID: `12345678-1234-5678-1234-56789abcdef0`
- Control Characteristic UUID: `12345678-1234-5678-1234-56789abcdef1`
- Commands: `start`, `stop`, `reset:<milliseconds>`
- Score notifications: `SCORE:` prefix for real-time scoring updates
- Acknowledgments: `ACK: <command>` for command confirmations

### UART Protocol (ESP32 <--> Arduino Mega)
- ESP32: Serial2 on pins GPIO16 (RX) and GPIO17 (TX)
- Arduino Mega: Serial1 at 115200 baud
- Commands flow from mobile app through ESP32 to Mega
- Score updates flow from Mega through ESP32 to mobile app

## Hardware Configuration

### ESP32 Pin Assignments
- Serial2 RX: GPIO16
- Serial2 TX: GPIO17
- BLE device name: "FencBox_01"

### Arduino Mega Pin Assignments
Fencer 1:
- A1_PIN: 22 (Input, weapon detection)
- B1_PIN: 24 (Output, ground reference)
- C1_PIN: 26 (Input, valid touch detection)

Fencer 2:
- A2_PIN: 23 (Input, weapon detection)
- B2_PIN: 25 (Output, ground reference)
- C2_PIN: 27 (Input, valid touch detection)

Outputs:
- LED1_PIN: 30 (Fencer 1 touch indicator)
- LED2_PIN: 32 (Fencer 2 touch indicator)
- BUZZER_PIN: 34 (Audio feedback)
- TM1637 Display: CLK=4, DIO=5 (Timer display)

Controls:
- BUTTON_PIN: 33 (Timer control button)

## Timing Configuration
- TOUCH_DELAY: 2000ms (output hold duration)
- DOUBLE_TOUCH_WINDOW: 40ms (simultaneous touch detection)
- Default timer: 180 seconds (3 minutes)

## Development Commands

### Arduino IDE Setup
1. Install ESP32 board support in Arduino IDE
2. Select appropriate board:
   - ESP32: "ESP32 Dev Module"
   - Mega: "Arduino Mega or Mega 2560"
3. Upload firmware to each board:
   - ESP32: `Firmware_ESP32_Comunication_base.ino`
   - Mega: `Henry_Fencing_Box_ESP32_communication_base.ino`

### Serial Monitoring
- ESP32: Monitor at 115200 baud for BLE connection status
- Mega: Monitor at 115200 baud for touch detection and timer status

## Firmware Features

### ESP32 Firmware
- BLE server with automatic reconnection
- UART bridge for command forwarding
- Message filtering for score notifications
- Connection state management

### Arduino Mega Firmware
- Fencing touch detection logic
- Timer management with TM1637 display
- Button controls for timer start/stop
- Serial command processing
- Score notification generation

## Testing Hardware Communication
1. Upload firmware to both boards
2. Connect boards via UART (TX to RX, RX to TX)
3. Use mobile app or BLE scanner to connect
4. Send commands and verify acknowledgments
5. Test touch detection with fencing equipment

## Important Notes
- Always ensure proper UART connections between boards
- BLE advertising restarts automatically on disconnect
- Timer commands must include millisecond values for reset
- Touch detection requires proper grounding setup