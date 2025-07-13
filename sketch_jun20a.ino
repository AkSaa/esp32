#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#define MQTT_MAX_PACKET_SIZE 1024
#include <PubSubClient.h>
#include <arduino_secrets.h>

#define LED_BUILTIN 8 // Internal LED pin is 8 as per schematic

// GPIO where the DS18B20 is connected to
const int oneWireBus = 7;     

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* mqtt_server = MQTT_SERVER;
const char* mqtt_user = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;

const char* state_topic = "homeassistant/esp32-c3-1/temperature";
const char* availability_topic = "homeassistant/esp32-c3-1/status";
const char* discovery_topic = "homeassistant/sensor/esp32-c3-1_temp/config";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);  
  
  // Start the DS18B20 sensor
  sensors.begin();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == "homeassistant/esp32-c3-1/output") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("on");
//      digitalWrite(ledPin, HIGH);
      digitalWrite(LED_BUILTIN, LOW);
      
    }
    else if(messageTemp == "off"){
      Serial.println("off");
//      digitalWrite(ledPin, LOW);
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32-Client-1", mqtt_user, mqtt_password, "homeassistant/esp32-c3-1/status", 1, true, "offline")) {
      client.publish("homeassistant/esp32-c3-1/status", "online", true);
      Serial.println("connected");
      delay(500);


      /*Discovery message ei toiminut
      Julkaistu MQTT Brokerilla:

      mosquitto_pub -h localhost -u MQTT_USER -P MQTT_PASSWORD -t homeassistant/sensor/esp32-c3-1_temp_v3/config -r -m '{
      "name": "ESP32 C3 Temperature",
      "state_topic": "homeassistant/esp32-c3-1/temperature",
      "unit_of_measurement": "°C",
      "device_class": "temperature",
      "unique_id": "esp32_c3_1_temp_sensor_v3",
      "availability_topic": "homeassistant/esp32-c3-1/status",
      "payload_available": "online",
      "payload_not_available": "offline",
      "device": {
        "identifiers": ["esp32-c3-1"],
        "name": "ESP32 C3 Temp MQTT",
        "model": "ESP32-C3 SuperMini",
        "manufacturer": "YourName"
        }
      }' 
      
      */

      // Send discovery message for temperature sensor
      String discoveryTopic = "homeassistant/sensor/esp32-c3-1_temp_v3/config";
      String payload =
        "{"
        "\"name\": \"ESP32 C3 Temperature\","
        "\"state_topic\": \"homeassistant/esp32-c3-1/temperature\","
        "\"unit_of_measurement\": \"°C\","
        "\"device_class\": \"temperature\","
        "\"unique_id\": \"esp32_c3_1_temp_sensor_v2\","
        "\"availability_topic\": \"homeassistant/esp32-c3-1/status\","
        "\"payload_available\": \"online\","
        "\"payload_not_available\": \"offline\","
        "\"device\": {"
        "\"identifiers\": \"esp32-c3-1\","
        "\"name\": \"ESP32 C3 Temp MQTT\","
        "\"model\": \"ESP32-C3 SuperMini\","
        "\"manufacturer\": \"YourName\""
        "}"
        "}";

      client.loop();
      delay(500);
      client.publish(discoveryTopic.c_str(), payload.c_str(), true); 
      Serial.println("Published payload.");
      Serial.println(payload);
//      client.subscribe("homeassistant/esp32-c3-1/output");
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// the loop function runs over and over again forever
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
  //  digitalWrite(LED_BUILTIN, LOW);   // turn the LED on (LOW because the LED is inverted)
    sensors.requestTemperatures(); 
    float temperatureC = sensors.getTempCByIndex(0);
    Serial.print(temperatureC);
    Serial.println("ºC");    
    
    char tempString[8];
    dtostrf(temperatureC, 1, 2, tempString);
    client.publish(state_topic, tempString, true);  // retain=true

    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);

    delay(5000);
  }
}