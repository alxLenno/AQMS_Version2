# Smart IoT Air Quality Monitoring System (AQMS v2)

An IoT-enabled Air Quality Monitoring System based on the Arduino Due (32-bit ARM Cortex-M3) designed for high-resolution environmental sensing and remote telemetry via GSM/GPRS.

---

## 🚀 Overview

This firmware acquires readings from multiple gas, particulate, and meteorological sensors, performs on-device calibration and offset compensation, displays the measurements on a graphical LCD, and transmits telemetry to a remote backend / MQTT broker using a SIM800L cellular modem.

### Key Capabilities
- **High-Resolution ADC Sampling:** Leverages the 12-bit ADC (0–4095) of the Arduino Due.
- **Multi-Sensor Suite:** Simultaneous tracking of particulate matter, gases, VOCs, NOx, and weather parameters.
- **Over-the-Air / Serial Calibration:** Supports two-point linear calibration stored persistently in Due flash memory (`DueFlashStorage`).
- **Cellular Telemetry:** SIM800L GPRS uplink supporting both HTTP REST ingestion and MQTT pub/sub for telemetry and calibration synchronization.
- **Local Visualization:** ST7920 128x64 graphical display powered by U8g2.

---

## 🛠 Hardware Architecture

### Microcontroller
- **Arduino Due** (Atmel SAM3X8E ARM Cortex-M3 @ 84 MHz, 3.3V logic)

### Sensors & Peripherals
| Sensor / Module | Metric | Interface / Pin |
|---|---|---|
| **PMS5003** | PM1.0, PM2.5, PM10 | Serial3 (UART, 9600 baud) |
| **MH-Z19C** | CO2 (NDIR) | Serial2 (UART, 9600 baud) |
| **Sensirion SGP41** | VOC & NOx Index | I2C (Wire) |
| **MQ-7** | Carbon Monoxide (CO) | Analog Pin A0 |
| **MQ-131** | Ozone (O3) | Analog Pin A1 |
| **Anemometer** | Wind Speed (0–30 m/s) | Analog Pin A2 (0–5V with divider) |
| **DHT11** | Temperature & Humidity | Digital Pin 7 |
| **DS3231** | Real-Time Clock | I2C (Wire) |
| **SIM800L** | 2G GPRS Modem | Serial1 (UART, 9600 baud) |
| **ST7920 (128x64)** | Graphic LCD Display | Software SPI (Pins 13, 11, 10, 8) |

---

## 📂 Project Structure

```text
├── Smart_Air_Quality_Monitoring_Sytsem.ino  # Main entry point: setup(), loop(), and task scheduling
├── config.h                                # System configuration, pin definitions, calibration structs
├── config.cpp                              # Calibration routines, defaults, and Flash storage persistence
├── sensors.h                               # Sensor subsystem interface and variable declarations
├── sensors.cpp                             # Sensor drivers, acquisition routines, and calibration math
├── display.h                               # Display manager definitions
├── display.cpp                             # U8g2 multi-page UI rendering
├── communication.h                         # GSM and MQTT network interfaces
├── communication.cpp                       # SIM800L driver, GPRS connection, MQTT callbacks, HTTP ingest
└── README.md                               # Project documentation
```

---

## ⚙️ Configuration & Calibration

### Calibration Equation
Sensor values undergo linear calibration:
$$\text{Calibrated Value} = (\text{Gain} \times \text{Raw Value}) + \text{Offset}$$

Calibration constants are retained in non-volatile flash memory and can be adjusted remotely via MQTT on `node/{DEVICE_ID}/calibration` or through the Serial interface:
```text
CAL:<CO_GAIN>,<CO_OFFSET>,<O3_GAIN>,<O3_OFFSET>,<CO2_GAIN>,<CO2_OFFSET>,<PM25_GAIN>,<PM25_OFFSET>,<TEMP_OFFSET>,<HUM_OFFSET>
```

---

## 📦 Required Libraries

- [U8g2](https://github.com/olikraus/u8g2)
- [DHT sensor library](https://github.com/adafruit/DHT-sensor-library)
- [RTClib](https://github.com/adafruit/RTClib)
- [MH-Z19](https://github.com/WifWaf/MH-Z19)
- [Sensirion I2C SGP41](https://github.com/Sensirion/arduino-i2c-sgp41)
- [Sensirion Gas Index Algorithm](https://github.com/Sensirion/arduino-gas-index-algorithm)
- [TinyGSM](https://github.com/vshymanskyy/TinyGSM)
- [PubSubClient](https://github.com/knolleary/pubsubclient)
- [ArduinoJson](https://arduinojson.org/)
- [DueFlashStorage](https://github.com/sebnil/DueFlashStorage)

---

## 📄 License

This project is developed as part of an MSc research project. All rights reserved.
