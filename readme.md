# ESP8266 MQTT 语音合成播报器 (TTS)

通过 ESP8266 连接 MQTT 服务器并订阅指定主题，收到英文文本后自动在本地完成 TTS 语音合成，并直接输出音频。

> **提示**：受限于 ESP8266 的硬件性能，本地 TTS 输出的音质无法媲美电脑端，但能达到清晰辨识英文单词的实用效果。

---

## 硬件需求

* **开发板**：ESP8266 开发板（如 NodeMCU）
* **功放模块**：LM386 音频功放模块
* **扬声器**：小喇叭（8Ω 0.5W/1W 等）
* **供电及连接线**：杜邦线、USB 供电线

---

<img width="518" height="327" alt="image" src="https://github.com/user-attachments/assets/76093163-b74e-4d46-97cf-fbc6601e6590" />

<img width="439" height="352" alt="image" src="https://github.com/user-attachments/assets/c6189ab9-9f99-4dcf-8ab5-1cf96742f37a" />

<img width="442" height="352" alt="image" src="https://github.com/user-attachments/assets/57cc42de-8860-4d9e-8c29-e6d9dc80295e" />

## 硬件接线指南

| ESP8266 引脚 | LM386 模块 / 喇叭 | 备注 |
| :--- | :--- | :--- |
| **GPIO3 (RX)** | IN (音频输入) | 音频信号输出引脚 |
| **GND** | GND | 必须共地 |
| **5V / 3.3V** | VCC | 视 LM386 模块支持供电电压而定 |
| - | OUT (音频输出) | 连接喇叭两端 |

---

## 编译与烧录

在 Arduino IDE 中打开工程，烧录前请根据实际环境修改以下配置：

1. **Wi-Fi 配置**：修改代码中的 Wi-Fi 名称（SSID）和密码。
2. **MQTT 服务器配置**：修改 MQTT Broker 的域名/IP、端口、用户名及密码。
3. **音量/增益调节（可选）**：在代码中搜索 `SetGain`，修改其参数值。
   * **增益范围**：`0` ~ `4`（数值越大，输出音量及增益越高）。

修改完成后，直接将程序烧录至 ESP8266 开发板即可。

---

## 使用说明

1. 启动 ESP8266，等待其成功连接 Wi-Fi 及 MQTT 服务器。
2. 使用任意 MQTT 客户端（如 MQTTX、MQTT Explorer 或手机 App）连接同一个 MQTT 服务器。
3. 向主题 `tts/say` 发送英文文本消息（例如：`Hello World`）。
4. ESP8266 收到消息后将自动通过喇叭播报音频。
