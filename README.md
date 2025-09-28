# 📡 IoT Smart Home - DS3231 + ESP32 + STM32

## 📖 Giới thiệu
Dự án IoT kết nối **ESP32**, **STM32**, và **Web Dashboard** để hiển thị thời gian thực, nhiệt độ, và điều khiển LED qua MQTT.

## ⚙️ Cấu trúc repo
iot-smart-home/
├── web/ # Web dashboard
├── esp32/ # ESP32 code (ESP-IDF)
├── stm32/ # STM32 code (CubeIDE project)
└── docs/ # Tài liệu
## 🚀 Hướng dẫn build

### 🔹 ESP32
```bash
cd esp32
idf.py build flash monitor