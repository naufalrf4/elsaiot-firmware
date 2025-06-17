# ELSA IoT - Environment Liquid Smart Analyzer

[![Firmware Version](https://img.shields.io/badge/Version-1.2.0-blue.svg)](https://github.com/elsaiot/firmware/releases)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

A smart water quality monitoring system for real-time analysis of pH, TDS, DO, and temperature parameters with IoT capabilities.

## 📋 Table of Contents
- [Features](#-features)
- [Hardware Requirements](#-hardware-requirements)
- [Installation](#-installation)
- [Configuration](#-configuration)
- [Usage](#-usage)
- [Documentation](#-documentation)
- [Contributing](#-contributing)
- [License](#-license)
- [Acknowledgements](#-acknowledgements)

## 🌟 Features
- 🎯 Multi-parameter sensing: pH, TDS, DO, Temperature
- 📶 WiFi & MQTT connectivity
- 📊 Local TFT display (320x240)
- 🔄 Over-the-Air (OTA) updates
- ⚙️ Remote calibration via web interface
- 📈 Data visualization dashboard
- 🔋 Low-power operation (3.3V DC)

## 🛠 Hardware Requirements
| Component              | Quantity | Specification                          |
|------------------------|----------|----------------------------------------|
| ESP32-S3 Dev Board     | 1        | Dual-core 240MHz, 8MB PSRAM           |
| ADS1115 ADC            | 1        | 16-bit resolution, 4 channels         |
| pH Sensor Probe        | 1        | Analog output (0-14pH)                |
| TDS Sensor             | 1        | 0-1000ppm range                       |
| Dissolved Oxygen Probe | 1        | Galvanic type (0-20mg/L)              |
| DS18B20 Temperature    | 1        | Waterproof stainless steel probe      |
| ILI9341 TFT Display    | 1        | 2.8" SPI interface                    |

## 📥 Installation

### Prerequisites
- Arduino IDE 2.0+ with ESP32 Board Support
- USB-C cable for programming
- Stable WiFi network (2.4GHz)

### Steps
1. Clone repository:
   ```
   git clone https://github.com/elsaiot/firmware.git
   cd firmware
   ```
2. Install required libraries:
   - Adafruit_ADS1X15 @2.4.0
   - PubSubClient @2.8
   - ArduinoJson @6.19
   - Adafruit_ILI9341 @1.5.6

3. Configure settings in `PrivateConfig.h`:
   ```
   #define MQTT_SERVER "mqtt.yourserver.com"
   #define MQTT_USER "elsa-user"
   #define MQTT_PASSWORD "secure-password"
   ```

## ⚙️ Configuration

### Pin Mapping
| ESP32 Pin | Connected Component       |
|-----------|---------------------------|
| GPIO21    | ADS1115 SDA               |
| GPIO20    | ADS1115 SCL               |
| GPIO5     | DS18B20 Data              |
| GPIO15    | TFT CS                    |
| GPIO2     | TFT DC                    |

### First-Time Setup
1. Power on device
2. Connect to ELSA-XXXX AP (password: 12345678)
3. Select your WiFi network in captive portal
4. Device reboots and connects to MQTT broker

## 🖥 Usage

### Uploading Firmware
```
arduino-cli compile --fqbn esp32:esp32:esp32s3 .
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32s3
```

### Calibration Workflow
1. Access web dashboard at `https://[your-server]/elsa`
2. Navigate to Device → Calibration
3. Follow on-screen instructions:
   - pH: 3-point calibration with buffer solutions
   - TDS: Single-point with 342ppm solution
   - DO: Air-saturated water calibration

### Monitoring Data
```
mosquitto_sub -h mqtt.yourserver.com -t "elsaiot/#" -u elsa-user -P secure-password
```

## 📚 Documentation
- [MQTT Protocol Specification](docs/mqtt.md)
- [Calibration Procedures](docs/calibration.md)
- [Hardware Schematics](docs/hardware.pdf)
- [API Documentation](docs/api.md)

## 📜 License
Distributed under MIT License. See `LICENSE` for more information.

## 🙏 Acknowledgements
- Adafruit Industries for sensor libraries
- PlatformIO for build system
- NTP Pool Project for time synchronization
