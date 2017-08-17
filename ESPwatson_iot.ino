/**
  IBM IoT Foundation managed Device

  Author: Ant Elder
  License: Apache License v2
*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient/releases/tag/v2.3
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson/releases/tag/v5.0.7

//-------- Customise these values -----------
const char* ssid = "YU5010A";
const char* password = "12345678Sub";

//const char* ssid = "curio";
//const char* password = "curio#123456";

#define ORG "0etuql"
#define DEVICE_TYPE "NodeMCU"
#define DEVICE_ID "EnkeNodeMCU"
#define TOKEN "lhNCCS879GQkWZd4+w"
//-------- Customise the above values --------

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

//evt when sending data
const char publishTopic[] = "iot-2/evt/status/fmt/json";
//cmd recieving command
const char EnkefaloTopic[] = "iot-2/cmd/test/fmt/String";
const char responseTopic[] = "iotdm-1/response";
const char manageTopic[] = "iotdevice-1/mgmt/manage";
const char updateTopic[] = "iotdm-1/device/update";
const char rebootTopic[] = "iotdm-1/mgmt/initiate/device/reboot";


void callback(char* topic, byte* payload, unsigned int payloadLength);
void doCommand(String cmd);
WiFiClient wifiClient;
PubSubClient client(server, 1883, callback, wifiClient);

int publishInterval = 40000; // 30 seconds
long lastPublishMillis;
int LED_STATUS=0; 

void setup() {
  Serial.begin(115200); Serial.println();
  pinMode(D1, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  wifiConnect();
  mqttConnect();
  initManagedDevice();
}

void loop() {
  if (millis() - lastPublishMillis > publishInterval) {
    publishData();
    lastPublishMillis = millis();
  }

  if (!client.loop()) {
    mqttConnect();
    //if mqtt sleeps then this function willhelp to restart and connect to all topics
    initManagedDevice();
  }
}

void wifiConnect() {
  Serial.print("Connecting to "); Serial.print(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("nWiFi connected, IP address: "); Serial.println(WiFi.localIP());
}

void mqttConnect() {
  if (!!!client.connected()) {
    Serial.print("Reconnecting MQTT client to "); Serial.println(server);
    while (!!!client.connect(clientId, authMethod, token)) {
      Serial.print(".");
      delay(500);
    }
    Serial.println();
  }
}

void initManagedDevice() {

  digitalWrite(D1, LOW);
  if (client.subscribe("iotdm-1/response")) {
    Serial.println("subscribe to responses OK");
  } else {
    Serial.println("subscribe to responses FAILED");
  }

  if (client.subscribe(rebootTopic)) {
    Serial.println("subscribe to reboot OK");
  } else {
    Serial.println("subscribe to reboot FAILED");
  }

  if (client.subscribe("iotdm-1/device/update")) {
    Serial.println("subscribe to update OK");
  } else {
    Serial.println("subscribe to update FAILED");
  }

  if (client.subscribe(EnkefaloTopic)) {
    Serial.println("subscribe to Enkefalo OK");
  } else {
    Serial.println("subscribe to Enkefalos FAILED");
  }

  StaticJsonBuffer<300> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& d = root.createNestedObject("d");
  JsonObject& metadata = d.createNestedObject("metadata");
  metadata["publishInterval"] = publishInterval;
  JsonObject& supports = d.createNestedObject("supports");
  supports["deviceActions"] = true;

  char buff[300];
  root.printTo(buff, sizeof(buff));
  Serial.println("publishing device metadata:"); Serial.println(buff);
  if (client.publish(manageTopic, buff)) {
    Serial.println("device Publish ok");
  } else {
    Serial.print("device Publish failed:");
  }
}

void publishData() {
//  String payload = "{\"d\":{\"counter\":";
//  payload += millis() / 1000;
//  payload += ",\"status\":";
//  payload += LED_STATUS;
//  payload += "}}";

    String payload = "{\"d\":{\"LEDSTATUS\":";
  payload += LED_STATUS;
  payload += "}}";


  Serial.print("Sending payload: "); Serial.println(payload);

  if (client.publish(publishTopic, (char*) payload.c_str())) {
    Serial.println("Publish OK");
  } else {
    Serial.println("Publish FAILED");
  }
}

void callback(char* topic, byte* payload, unsigned int payloadLength) {
  Serial.print("callback invoked for topic: "); Serial.println(topic);

if (strcmp (EnkefaloTopic, topic) == 0) {
  Serial.println("Enkefalos recieved...");
 // Serial.println((char*)payload);
   String cmd = "";
  for (int i = 0; i < payloadLength; i++) {
    Serial.print((char)payload[i]);
    cmd +=(char)payload[i];
  }
  doCommand(cmd); 
  }
  else{
    Serial.println("Enkefalos Topic NOT recieved..."); 
  }

  if (strcmp (responseTopic, topic) == 0) {
    return; // just print of response for now
  }

  if (strcmp (rebootTopic, topic) == 0) {
    Serial.println("Rebooting...");
    ESP.restart();
  }

  if (strcmp (updateTopic, topic) == 0) {
    handleUpdate(payload);
  }
  
}

void doCommand(String cmd) {
// int currentState = digitalRead(DEVICE_RELAY);
// int newState = (cmd == "on");
 if(cmd == "on")
 {
   LED_STATUS=1;
   digitalWrite(D1, HIGH);   
   Serial.println(" STOVE IS ON");
   
 } else if(cmd == "off")
 {
   LED_STATUS=0;
   digitalWrite(D1, LOW);
   delay(1000); 
   digitalWrite(D1, HIGH);  // Turn the LED off by making the voltage HIGH
                       // Wait for two seconds (to demonstrate the active low LED)
   Serial.println(" STOVE IS OFF");
 }
// digitalWrite(DEVICE_RELAY, newState);
// Serial.print("Relay switched from "); 
// Serial.print(currentState ? "On" : "Off");Serial.print(" to "); Serial.println(newState ? "On" : "Off");
}

void handleUpdate(byte* payload) {
  StaticJsonBuffer<300> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject((char*)payload);
  if (!root.success()) {
    Serial.println("handleUpdate: payload parse FAILED");
    return;
  }
  Serial.println("handleUpdate payload:"); root.prettyPrintTo(Serial); Serial.println();

  JsonObject& d = root["d"];
  JsonArray& fields = d["fields"];
  for (JsonArray::iterator it = fields.begin(); it != fields.end(); ++it) {
    JsonObject& field = *it;
    const char* fieldName = field["field"];
    if (strcmp (fieldName, "metadata") == 0) {
      JsonObject& fieldValue = field["value"];
      if (fieldValue.containsKey("publishInterval")) {
        publishInterval = fieldValue["publishInterval"];
        Serial.print("publishInterval:"); Serial.println(publishInterval);
      }
    }
  }
}
