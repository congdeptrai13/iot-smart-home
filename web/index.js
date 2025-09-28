    const client = mqtt.connect("ws://broker.hivemq.com:8000/mqtt");

    // Update LED icon
    function updateLEDIcon(state) {
      const ledIcon = document.getElementById("led-icon");
      if (state === 1 || state === "1" || state === "ON") {
        ledIcon.textContent = "💡 ON";
        ledIcon.style.color = "gold";
      } else {
        ledIcon.textContent = "💡 OFF";
        ledIcon.style.color = "gray";
      }
    }

    // Gửi lệnh LED
    function sendLED(state) {
      const cmd = "UPDATE_LED:" + state;
      client.publish("esp32/cmd", cmd);
      console.log("➡ Sent:", cmd);
    }

    // Hiển thị trạng thái kết nối
    function setStatus(isConnected) {
      const statusDiv = document.getElementById("status");
      if (isConnected) {
        statusDiv.textContent = "🟢 Connected to MQTT Broker";
        statusDiv.className = "status connected";
      } else {
        statusDiv.textContent = "🔴 Disconnected";
        statusDiv.className = "status disconnected";
      }
    }

    // Pad số < 10 với 0
    function pad(num) {
      return num.toString().padStart(2, "0");
    }

    client.on("connect", () => {
      setStatus(true);

      // Subscribe các topic cần thiết
      client.subscribe("esp32/datetime");
      client.subscribe("esp32/temperature");
      client.subscribe("esp32/LEDState");

      // Yêu cầu ESP32 gửi lại trạng thái LED khi UI mới start
      client.publish("esp32/cmd", "GET_LED_STATE");

      // Request datetime mỗi 1s
      setInterval(() => {
        client.publish("esp32/cmd", "UPDATE_DATETIME");
      }, 1000);

      // Request nhiệt độ mỗi 10s
      setInterval(() => {
        client.publish("esp32/cmd", "UPDATE_TEMP");
      }, 10000);
    });

    client.on("close", () => {
      setStatus(false);
    });

    client.on("message", (topic, message) => {
      try {
        const data = JSON.parse(message.toString());

        if (topic === "esp32/datetime") {
          document.getElementById("time").textContent =
            `${pad(data.hour)}:${pad(data.min)}:${pad(data.sec)}`;
          document.getElementById("date").textContent =
            `${pad(data.date)}/${pad(data.month)}/${data.year}`;
        }

        if (topic === "esp32/temperature") {
          document.getElementById("temp").textContent = data.temp.toFixed(2);
        }

        if (topic === "esp32/LEDState") {
          updateLEDIcon(data.LED);
        }
      } catch (e) {
        console.error("Invalid JSON:", message.toString(), e);
      }
    });