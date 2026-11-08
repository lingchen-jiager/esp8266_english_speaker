/*
 * ESP8266 MQTT 离线语音播报器（本地 TTS）
 * --------------------------------
 * 链路：MQTT(tts/say 收到英文文字) -> SAM 引擎本地合成（无服务器、无网络 TTS）
 *       -> ESP8266Audio delta-sigma 输出(GPIO3/RX) -> RC 低通 -> LM386 -> 喇叭
 *
 * 主题约定：
 *   订阅 tts/say    : 任意英文文字（建议 <=200 字符），收到即朗读
 *   发布 tts/status : online / speaking / done / offline(LWT) / 错误信息
 *
 * 库依赖（均已装入 Documents/Arduino/libraries）：
 *   - PubSubClient 2.8
 *   - ESP8266Audio
 *   - ESP8266SAM (earlephilhower)
 *
 * 硬件接线：
 *   GPIO3(RX) --[1kΩ]--+--[100nF 对地]--+--> LM386 模块 IN
 *   NodeMCU 5V(VIN) ----> LM386 VCC
 *   NodeMCU GND  ------> LM386 GND（必须共地）
 *   喇叭 --------------> LM386 绿色端子
 *   注意：烧录时先拔掉 RX 上的音频线，烧完再接回！
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <AudioOutputI2SNoDAC.h>
#include <ESP8266SAM.h>

// ============ 配置区：烧录前必改 WiFi！============
const char* WIFI_SSID = "your_wifi";
const char* WIFI_PASS = "your_password";
// ==================================================

const char* MQTT_HOST = "your_mqtt_server_ip_or_domain"; 
const int   MQTT_PORT = 1883;  //你的端口，一般是1883
const char* MQTT_USER = "your_user";
const char* MQTT_PASS = "your_password";

const char* TOPIC_SAY    = "tts/say";
const char* TOPIC_STATUS = "tts/status";

#define MAX_TEXT_LEN 200   // 播报文字长度上限

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

AudioOutputI2SNoDAC* out = nullptr;
ESP8266SAM* sam = nullptr;

String pendingText = "";   // 待播报文字（在 loop 中处理，回调里只存不播）
bool speaking = false;

// ---------------- 播报 ----------------

void speakText(const String& text) {
  speaking = true;
  mqtt.publish(TOPIC_STATUS, "speaking");
  Serial.printf("[tts] speaking: %s\n", text.c_str());
  // SAM 合成 + 播放是阻塞调用，短句几秒内完成
  sam->Say(out, text.c_str());
  speaking = false;
  mqtt.publish(TOPIC_STATUS, "done");
  Serial.println("[tts] done");
}

// ---------------- MQTT ----------------

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  msg.reserve(length);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();
  Serial.printf("[mqtt] %s: %s\n", topic, msg.c_str());

  if (strcmp(topic, TOPIC_SAY) == 0) {
    if (msg.length() == 0) return;
    if (msg.length() > MAX_TEXT_LEN) msg = msg.substring(0, MAX_TEXT_LEN);
    pendingText = msg;   // 只记录，真正播报放到 loop()
  }
}

void mqttConnect() {
  while (!mqtt.connected()) {
    String clientId = "esp8266-sam-" + String(ESP.getChipId(), HEX);
    Serial.print("[mqtt] connecting...");
    // LWT：异常掉线时自动发布 offline（保留消息）
    bool ok = mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS,
                           TOPIC_STATUS, 0, true, "offline");
    if (ok) {
      Serial.println("ok");
      mqtt.subscribe(TOPIC_SAY);
      mqtt.publish(TOPIC_STATUS, "online", true);
    } else {
      Serial.printf("failed, rc=%d, retry in 3s\n", mqtt.state());
      delay(3000);
    }
  }
}

// ---------------- 主流程 ----------------

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== ESP8266 SAM TTS Speaker ===");

  // 音频输出：I2S delta-sigma，固定输出在 GPIO3(RX)
  out = new AudioOutputI2SNoDAC();
  out->SetGain(2.5);      // 0.0~4.0，嫌声音小可调大
  out->begin();
  sam = new ESP8266SAM();
  sam->SetVoice(ESP8266SAM::VOICE_SAM);   // 可换 VOICE_ROBOT / VOICE_ET 等

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[wifi] connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n[wifi] connected, IP: %s\n", WiFi.localIP().toString().c_str());

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setKeepAlive(120);   // 播报期间 loop 会阻塞，放宽心跳
  mqtt.setSocketTimeout(5);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(500);
    return;
  }
  if (!mqtt.connected()) mqttConnect();
  mqtt.loop();

  if (pendingText.length() > 0 && !speaking) {
    String text = pendingText;
    pendingText = "";
    speakText(text);
  }
}
