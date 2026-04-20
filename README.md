# FrostByte – ESP32 Temperature-Controlled Relay with M2302 (DHT22)

This project uses an ESP32 development board together with an M2302 (DHT22) temperature and humidity sensor to control a relay. The relay switches ON or OFF based on the average temperature over the last 300 seconds, using hysteresis to avoid rapid toggling.

## 🔧 Hardware Components

- ESP32 Development Board (e.g. ESP32 DevKit V1)
- M2302 (DHT22) temperature & humidity sensor
- 1-channel relay module (5V, low-level trigger recommended)
- 5V power supply (USB or external)
- Jumper wires

## ⚙️ Functionality

- Takes one temperature reading every 10 seconds
- Stores the latest 30 readings (≈ 5 minutes)
- Calculates the average temperature
- Relay turns ON when average temperature > 5.0°C
- Relay turns OFF when average temperature < 3.0°C
- No switching occurs between thresholds (hysteresis)

## 🌐 Wi-Fi Access Point + Web Interface

The ESP32 runs in Access Point (AP) mode, allowing direct connection without a router.

- Wi-Fi name: FrostByte
- Password: fridgelord (change in code if needed)
- Connect via phone or PC
- Open browser and go to: http://192.168.4.1

The web interface shows:
- Average temperature
- Relay state (ON/OFF)

## 📦 PlatformIO Configuration (platformio.ini)

```ini
[env:esp32dev]
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
upload_speed = 115200
monitor_speed = 115200
upload_port = COM3

lib_deps =
    ESP32Async/ESPAsyncWebServer
    ESP32Async/AsyncTCP
```

## 🔌 Wiring & Connections

### ESP32 to M2302 (DHT22)

| DHT22 Pin | ESP32 Pin | Notes                 |
|----------|-----------|-----------------------|
| VCC      | 3.3V      | Do NOT use 5V         |
| GND      | GND       | Common ground         |
| DATA     | GPIO4     | Any digital pin works |

Add a 10k pull-up resistor between DATA and VCC if needed.

### ESP32 to Relay Module

| Relay Pin | ESP32 Pin |
|----------|-----------|
| VCC      | 5V        |
| GND      | GND       |
| IN       | GPI23     |

## ⚠️ Safety Warning

The relay may switch high voltage (e.g. fridge compressor):
- Use proper insulation
- Never touch live wires
- Ensure correct wiring of COM / NO / NC terminals

## 🪛 Power Supply Notes

- ESP32 can be powered via USB
- Relay module typically requires 5V
- DHT22 runs on 3.3V
- Ensure common ground between all components

## 📚 Notes

- Uses async web server for responsive UI
- Designed for stable temperature control
- You can adjust thresholds, averaging duration, and Wi-Fi credentials

## ⚠️ Hardware Reliability Note (ESP-01S)

During early development, an **ESP-01S (ESP8266)** board was used, but it showed unstable behavior:
- The device would sometimes run correctly, but **stop responding after a few hours**
- Wi-Fi and relay control became unreliable over time
- Reboots and reflashing did not consistently fix the issue

After switching to an **ESP32 development board**, all issues disappeared and the system became stable.

The ESP-01S board was likely defective or suffering from power instability (a common issue with ESP-01 modules).

If you encounter similar problems: upgrade to ESP32 for better stability

Unreliable hardware can mimic software bugs, so it’s worth ruling this out early.