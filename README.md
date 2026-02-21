# FrostByte - ESP-01S Temperature-Controlled Relay with M2302 (DHT22)

This project uses an ESP-01S Wi-Fi module together with an M2302 (DHT22) temperature and humidity sensor to control a relay. The relay switches ON or OFF based on the **average temperature over the last 300 seconds**, using hysteresis to avoid rapid toggling.

## 🔧 Hardware Components

- ESP-01S Module (ESP8266)
- M2302 (DHT22) temperature & humidity sensor
- ESP-01S 1-Channel Relay Module, also called ESP-01S / ESP01S Relay Daughter Board. It has two screw terminals and a 2x4 pin-header compatible with ESP-01S Module.
- 5V power supply for the relay module, also powering the sensor and ESP-01
- Wires

## ⚙️ Functionality

- Takes one temperature reading every 10 seconds.
- Stores the latest 30 readings (≈ 5 minutes).
- Calculates the average temperature from the buffer.
- **Relay turns ON** when average temperature > **5.0°C**.
- **Relay turns OFF** when average temperature < **3.0°C**.
- No switching occurs between 5.0°C and 7.0°C (hysteresis zone).

## 🌐 Wi-Fi Access Point + Web Interface

The ESP-01S can act as its own Wi-Fi access point (AP) so you can connect to it directly (without an internet router) and monitor the temperature in a web browser.

- The ESP-01S starts in AP mode with a name _FrostByte_ and password _fridgelord_. These can be changed in the code.
- You can connect to it from your phone or computer via Wi-Fi.
- After connecting, open a browser and go to http://192.168.4.1
- The ESP will serve a simple HTML page showing the current average temperature and relay state.

## 📦 PlatformIO Configuration (`platformio.ini`)

```ini
[env:esp01_1m]
[env:esp01_1m]
platform = espressif8266
board = esp01_1m
framework = arduino
monitor_speed = 9600
lib_compat_mode = strict
lib_ldf_mode = chain
lib_deps =
  ESP32Async/ESPAsyncTCP
  ESP32Async/ESPAsyncWebServer
upload_port = COM3
```
Information on how to program an **ESP-01S** is available somewhere else.  It is configured to have the programmer at port COM3.

Using the more stable **myDHT(()) library to support the DHT22 on Arduino available at https://github.com/tonimatutinovic/myDHT as a Git Submodule.

## 🔌 Wiring & Connections

Below are the required connections to set up the **ESP-01S** with the **M2302 (DHT22)** sensor and the **ESP-01S relay module**.

### 🧩 ESP-01S to M2302 (DHT22)

You need to solder these directly on the **ESP-01S relay module**, as you slot the **ESP-01S** in the 8-pin connector of the **ESP-01S relay module**.  Or splice three wires while you connect the **ESP-01S** and **ESP-01S relay module** with 8 wires.

| M2302 Pin | Connects To (ESP-01) | Notes                             |
|-----------|----------------------|-----------------------------------|
| VCC       | 3.3V                 |                                   |
| GND       | GND                  | Common ground                     |
| DATA      | GPIO2                |                                   |

### ⚡ ESP-01S Relay Module

You need to power it with 5V, and use the relay contacts to control/switch the fridge compressor. **WARNING**: High voltage!

### 🪛 Power Supply Notes

- **ESP-01S:** Requires a **regulated 3.3V** power supply, that is already included in the **ESP-01S relay module**, which requires 5V.
- **Relay module:** Needs **5V** to power the relay coil.  That is supplied in to the **ESP-01S relay module**.
- **M2302 (DHT22):** Runs at 3.3V, not 5V.
- Do **not** power the ESP-01S directly from USB or unregulated 5V.
