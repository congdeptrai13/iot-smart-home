# 📡 Smart Home System – System Architecture

## 1. Overview
This project is a **Smart Home IoT System** that integrates:
- **STM32**: Reads time (DS3231) and temperature, controls LED.
- **ESP32**: Handles WiFi + MQTT, bridges STM32 ↔ Server.
- **Web Dashboard**: UI to monitor data and control devices.
- **MQTT Broker**: Central hub for message exchange.

---

## 2. Block Diagram
[STM32] --UART--> [ESP32] --WiFi/MQTT--> [MQTT Broker] <--WS--> [Web Dashboard]

yaml
Copy code

- STM32: Reads sensors & executes LED commands.  
- ESP32: Connects to WiFi, sends/receives via MQTT.  
- Web: Subscribes topics, shows data, sends commands.  
- Broker: Manages publish/subscribe.

---

## 3. Data Flow
1. **Server/Web → ESP32**  
   - Send command via `esp32/cmd` (e.g., `UPDATE_LED:ON`).  
   - ESP32 forwards to STM32 via UART.  

2. **STM32 → ESP32 → Server**  
   - STM32 responds with LED state (ON/OFF), time, or temperature.  
   - ESP32 publishes to:
     - `esp32/datetime`
     - `esp32/temperature`
     - `esp32/LEDState`

---

## 4. MQTT Topics
| Topic              | Direction         | Payload Example                         | Description               |
|--------------------|------------------|-----------------------------------------|---------------------------|
| `esp32/cmd`        | Web → ESP32      | `UPDATE_LED:ON` / `UPDATE_TEMP`         | Control commands          |
| `esp32/datetime`   | ESP32 → Web      | `{ "hour": 12, "min": 30, "sec": 45 }` | Current time              |
| `esp32/temperature`| ESP32 → Web      | `{ "temp": 28.50 }`                     | DS3231 temperature        |
| `esp32/LEDState`   | ESP32 → Web      | `{ "LED": 1 }`                          | LED status (1=ON, 0=OFF)  |

---

## 5. UART Protocol (ESP32 ↔ STM32)
Frame format:
[START_BYTE][CMD][DATA...][END_BYTE]

pgsql
Copy code

- `CMD_TIME` → Request time  
- `CMD_TEMP` → Request temperature  
- `CMD_LED`  → Control LED (ON/OFF)  
- Response includes corresponding data.

---

## 6. System Workflow
```
sequenceDiagram
    participant Web
    participant MQTT
    participant ESP32
    participant STM32

    Web->>MQTT: Publish UPDATE_LED:ON
    MQTT->>ESP32: Forward command
    ESP32->>STM32: UART CMD_LED(ON)
    STM32->>ESP32: LED state = 1
    ESP32->>MQTT: Publish {"LED":1} on esp32/LEDState
    MQTT->>Web: Forward {"LED":1}
    Web->>Web: Update icon 💡 ON
7. Future Extension
Add multiple sensors (humidity, motion).

Store data in database (InfluxDB, Firebase).

Add authentication & TLS for MQTT.

Mobile app integration.

