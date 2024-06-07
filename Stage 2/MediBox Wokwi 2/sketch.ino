#include <PubSubClient.h>
#include <WiFi.h>
#include "DHTesp.h"

#include <WiFiUdp.h>
#include <ESP32Servo.h>

const int servoPin = 18;
int pos = 0;
int t_off = 30;
float gamma_i = 0.75;

Servo servo;

float left;
float right;

const int DHTPin = 15;
#define LED_BUILTIN 12

WiFiClient espClient;
PubSubClient mqttClient(espClient);

DHTesp dhtSensor;

char tempArr[6];
char leftldr[4];
char rightldr[4];

// bool isScheduledON = false;
// unsigned long scheduledOnTime;

void setup() {
  Serial.begin(115200);
  dhtSensor.setup(DHTPin, DHTesp::DHT22);
  setupWiFi();
  setupMqtt();

  // timeClient.begin();
  // timeClient.setTimeOffset(5.5 * 3600);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  pin_setup();
}

void loop() {
  if (!mqttClient.connected()) {
    Serial.println("Reconnecting to MQTT Broker");
    connectTOBroker();
  }

  mqttClient.loop();

  updateTemp();
  Serial.println(tempArr);
  mqttClient.publish("TEMP_READINGS_GOT", tempArr);
    // checkSchedule();
  delay(1000);

  // for (pos = 0; pos <= 180; pos += 1) {
  //   servo.write(pos);
  //   delay(15);
  // }
  // for (pos = 180; pos >= 0; pos -= 1) {
  //   servo.write(pos);
  //   delay(15);
  // }

  ldr_values();
  servo_set();
}

void setupWiFi() {
  WiFi.begin("Wokwi-GUEST", "");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Wifi Connected");
  Serial.print("IP Adderss: ");
  Serial.println(WiFi.localIP());
}

void setupMqtt() {
  mqttClient.setServer("test.mosquitto.org", 1883);
  mqttClient.setCallback(recieveCallback);
}

void connectTOBroker() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("ESP32Client-sdfsjdfsdfsdf")) {
      Serial.println("MQTT Connected");
      // mqttClient.subscribe("ENTC-ON-OFF_NI");
      // mqttClient.subscribe("ENTC-ADMIN-SCH-ON_NI");
      mqttClient.subscribe("MIN_ANGLE_ADJ_SERVO_SLIDER");
      mqttClient.subscribe("CONTROL_FACTOR_SERVO_ANGLE_SLIDER");
    } else {
      Serial.print("Failed To connect to MQTT Broker");
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
}

void updateTemp() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  String(data.temperature, 2).toCharArray(tempArr, 6);
}

void recieveCallback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char payloadCharAr[length];
  Serial.print("Message Recieved: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    payloadCharAr[i] = (char)payload[i];
  }
  Serial.println();
  if (strcmp(topic, "ENTC-ON-OFF_NI") == 0) {
    if (payloadCharAr[0] == '1') {
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
  // } else if (strcmp(topic, "ENTC-ADMIN-SCH-ON_NI") == 0) {
  //   if (payloadCharAr[0] == 'N') {
  //     isScheduledON = false;
      else if (strcmp(topic, "MIN_ANGLE_ADJ_SERVO_SLIDER") == 0) {
        t_off = String(payloadCharAr).toInt();
        Serial.print(t_off);
        Serial.print(" ");
    } else if (strcmp(topic, "CONTROL_FACTOR_SERVO_ANGLE_SLIDER") == 0) {
        gamma_i = String(payloadCharAr).toFloat();
        Serial.print(gamma_i);
        Serial.print(" ");
    }
  // } else {
  //     isScheduledON = true;
  //     scheduledOnTime = atol(payloadCharAr);
  //   }
  }


// unsigned long getTime() {
//   timeClient.update();
//   return timeClient.getEpochTime();
// }

// void checkSchedule() {
//   if (isScheduledON) {
//     unsigned long currentTime = getTime();
//     if (currentTime > scheduledOnTime) {
//       isScheduledON = false;
//             mqttClient.publish("ENTC-ADMIN-SCH-ESP-ON_NI", "0");
//       mqttClient.publish("ENTC-ADMIN-MAIN-ON-OFF-ESP_NI", "1");

//       Serial.println("Scheduled ON");
//       digitalWrite(LED_BUILTIN,HIGH);
//     }
//   }
// }

void ldr_values(){

  left = (float)(analogRead(34) - 4063)/(float)(-4031); //left ldr
  right = (float)(analogRead(35) - 4063)/(float)(-4031); //right ldr
  String(left,2).toCharArray(leftldr, 4);
  String(right,2).toCharArray(rightldr, 4);
  mqttClient.publish("LDR_LEFT_SENSOR_OUTPUT_ESP",leftldr);
  mqttClient.publish("LDR_RIGHT_SENSOR_OUTPUT_ESP",rightldr);
  // Serial.println(left);
  delay(100);

  // return left,right;
}

void servo_set(){
  float theta_motor;
  float max_intens;
  float D;

  if (left<right){
    D = 0.5;
    max_intens = right;
  }
  else{
    D = 1.5;
    max_intens = left;
  }

  theta_motor = min(t_off*D + (180 - t_off)*max_intens*gamma_i,(float)(180));
  Serial.println(theta_motor);

  servo.write(theta_motor);
  delay(100);
}

void pin_setup(){
  pinMode(34, INPUT); // left ldr
  pinMode(35, INPUT); // right ldr
  servo.attach(servoPin,500,2400);
}

