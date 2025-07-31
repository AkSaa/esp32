#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
extern "C" {
	#include "freertos/FreeRTOS.h"
	#include "freertos/timers.h"
}
#include <AsyncMQTT_ESP32.h>
#include <arduino_secrets.h>
#include <Preferences.h>
Preferences preferences;

#define mqtt_port 1883
#define LED_BUILTIN 8 
#define ONE_WIRE_BUS 3

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* mqtt_server = MQTT_SERVER;
const char* mqtt_user = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;

const char* state_topic = "homeassistant/esp32-c3-1/temperature";
const char* availability_topic = "homeassistant/esp32-c3-1/status";
const char* discovery_topic = "homeassistant/sensor/esp32-c3-1_temp_v4/config";

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

float tempC = 0.0;

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
    // Näytä vilkaisun LEDillä
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
}

void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch(event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        connectToMqtt();
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        xTimerStop(mqttReconnectTimer, 0);
        xTimerStart(wifiReconnectTimer, 0);
        break;
    }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);

  
  bool discoverySent = preferences.getBool("discoverySent", false);



  Serial.print("Session present: ");
  Serial.println(sessionPresent);

  
  delay(500);
    if (!discoverySent) {
    // Discovery payload
    String payload =
      "{"
      "\"name\": \"ESP32 C3 Temperature\","
      "\"state_topic\": \"" + String(state_topic) + "\","
      "\"unit_of_measurement\": \"°C\","
      "\"device_class\": \"temperature\","
      "\"unique_id\": \"esp32_c3_1_temp_sensor_v4\","
      "\"availability_topic\": \"" + String(availability_topic) + "\","
      "\"payload_available\": \"online\","
      "\"payload_not_available\": \"offline\","
      "\"expire_after\": 960,"
      "\"device\": {"
      "\"identifiers\": \"esp32-c3-1\","
      "\"name\": \"ESP32 C3 Temp MQTT\","
      "\"model\": \"ESP32-C3 SuperMini\","
      "\"manufacturer\": \"YourName\""
      "}"
      "}";

    mqttClient.publish(discovery_topic, 0, true, payload.c_str());
    Serial.println("Published discovery payload.");
    Serial.println(payload);

    preferences.putBool("discoverySent", true);  
  }
  mqttClient.publish(availability_topic, 0, true, "online");
  mqttPublishTemperature();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  qos: ");
  Serial.println(properties.qos);
  Serial.print("  dup: ");
  Serial.println(properties.dup);
  Serial.print("  retain: ");
  Serial.println(properties.retain);
  Serial.print("  len: ");
  Serial.println(len);
  Serial.print("  index: ");
  Serial.println(index);
  Serial.print("  total: ");
  Serial.println(total);
}

void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void mqttPublishTemperature() {
  sensors.requestTemperatures();
  tempC = sensors.getTempCByIndex(0);

  if (tempC == DEVICE_DISCONNECTED_C || tempC < -50.0 || tempC > 80.0) {
    Serial.println("⚠️ Anturia ei löydetty tai lukeminen epäonnistui.");
    return; 
  }

  char tempString[8];
  dtostrf(tempC, 1, 2, tempString);
  mqttClient.publish(state_topic, 0, true, tempString);
  Serial.print("Publishing at QoS 0, ");
  Serial.println(tempC);

  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);

  delay(100);
  waitTemperature();
}

void waitTemperature() {
  sensors.requestTemperatures();
  tempC = sensors.getTempCByIndex(0);

  uint64_t sleepMinutes;
  if (tempC >= 20.0 && tempC <= 80.0) { // Adjust if needed
    sleepMinutes = 1;  // Rising temperature, short sleep. Adjust as needed.
  } else {
    sleepMinutes = 10; // Normal temperature, longer sleep. Adjust as needed.
  }
  uint64_t sleepTimeUs = sleepMinutes * 60ULL * 1000000ULL;


  mqttClient.disconnect();
  WiFi.disconnect(true);

  Serial.println("Going to sleep.");
  Serial.print("Sleep duration (min): ");
  Serial.println(sleepMinutes);
  Serial.flush();
  esp_sleep_enable_timer_wakeup(sleepTimeUs);
  delay(500);
  esp_deep_sleep_start();
  Serial.println("Waking up.");
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  delay(100);

  preferences.begin("mqtt", false); 
  
  sensors.begin();

  Serial.println("Booting...");
  Serial.println();
  Serial.println();

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCredentials(mqtt_user, mqtt_password);

  connectToWifi();
}

void loop() {
}