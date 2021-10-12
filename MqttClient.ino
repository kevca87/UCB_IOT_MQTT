#include <WiFi.h>
#include <PubSubClient.h>

const char* WIFI_SSID = "HUAWEI-2.4G-M6xZ";
const char* WIFI_PASS = "HT7KU2Xv";
const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;

const char* CLIENT_ID = "esp32/1"; // unique client id
const char* OUT_TOPIC_DIST = "ucb/iot/group7/esp32/1/distance"; // publish for distanceSensor
const char* IN_TOPIC_RED_LED = "ucb/iot/group7/esp32/1/leds/red"; // read from redLed

const int ECHO_PIN = 12;
const int TRIGGER_PIN = 14;

const int LED_RED = 13;
const int LED_YELLOW = 0;
const int LED_GREEN = 4;

//This allows extense the number of LEDs
const int leds[] = {LED_RED, LED_YELLOW, LED_GREEN};

void turnOnLed(int ledPin){
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
}

void turnOffLed(int ledPin){
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void turnOnOneLed(int ledPin){
  for (int i = 0; i < sizeof(leds); i++){
    if (ledPin == leds[i]){
      turnOnLed(ledPin);
    } else{
      turnOffLed(leds[i]);
    }
  }
}

long readUltrasonicDistance(int triggerPin, int echoPin){
  pinMode(triggerPin, OUTPUT);  // Clear the trigger
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  // Sets the trigger pin to HIGH state for 10 microseconds
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  pinMode(echoPin, INPUT);
  // Reads the echo pin, and returns the sound wave travel time in microseconds
  return pulseIn(echoPin, HIGH);
}

float getDistance(){
  // measure the ping time in cm
  float distance = 0.01723 * readUltrasonicDistance(TRIGGER_PIN, ECHO_PIN);
  return distance;
}


WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void connectWiFi()
{
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Couldn't connect to WiFi.");
    while(1) delay(100);
  }
  Serial.println("Connected to " + String(WIFI_SSID));
}


void redLedBehavior(const char* topic,String message)
{
  if (String(topic) == IN_TOPIC_RED_LED) {
    //Serial.println("Message value: " + String(IN_TOPIC_RED_LED) + ":" + message[0]);
    if(String(message[0]) == String("1")){
      turnOnOneLed(LED_RED);
    }
    else if(String(message[0]) == String("0")) 
    {
      turnOffLed(LED_RED);
    }
    String timestampUnix = message.substring(1);
    Serial.println(timestampUnix);
  }
}

// PubSubClient callback function
void callback(const char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += String((char) payload[i]);
  }
  redLedBehavior(topic,message);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);  
  connectWiFi();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(callback);
}

boolean mqttClientConnect() {
  Serial.println("Connecting to MQTT broker...");
  if (mqttClient.connect(CLIENT_ID)) {
    Serial.println("Connected to " + String(MQTT_BROKER));

    mqttClient.subscribe(IN_TOPIC_RED_LED);
    Serial.println("Subscribed to " + String(IN_TOPIC_RED_LED));

  } 
  else {
    Serial.println("Couldn't connect to MQTT broker.");
  }
  return mqttClient.connected();
}


void publish(String message,const char* topic)
{
  mqttClient.publish(topic, message.c_str());
}


void publishDistance(float distance)
{
  publish(String(distance),OUT_TOPIC_DIST);
}

unsigned long previousConnectMillis = 0;
unsigned long previousPublishMillis = 0;

unsigned char counter = 0;

void loop() {
  unsigned long now = millis();
  if (!mqttClient.connected()) {
    // Connect to MQTT broker
    if (now - previousConnectMillis >= 5000) {
      previousConnectMillis = now;
      if (mqttClientConnect()) {
        previousConnectMillis = 0;
      } else delay(1000);
    }
  } else {
    // This should be called regularly to allow the client to process incoming messages and maintain its connection to the server
    mqttClient.loop();
    delay(100);

    if (now - previousPublishMillis >= 1000) {
      previousPublishMillis = now;

      float distance = getDistance();//[cm]
      publishDistance(distance);
      delay(100);
    }
  }
}
