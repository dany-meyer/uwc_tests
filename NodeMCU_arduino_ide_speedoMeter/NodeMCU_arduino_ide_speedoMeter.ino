#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoMqttClient.h>
#include <TinyGPS++.h>
#include <ArduinoJson.h>
#include <string.h>
#include <SoftwareSerial.h>

TinyGPSPlus gps;
/* TX --> D2, RX --> D1*/
SoftwareSerial SerialGPS(4,5); 

bool debug = true;

WiFiManager wifiManager;
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "broker.hivemq.com"; //"test.mosquitto.org";
int        port     = 1883;
String topic_str = "ima_test";

const long interval = 5000;
unsigned long previousMillis = 0;

static byte mymac[] = { 0x00,0x00,0x00,0x00,0x00,0x00 };
String myMacAdr = "";

void wifi_reconnect() {
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("Connection lost");
    while (WiFi.status() != WL_CONNECTED){
      uint8_t timeout = 8;
      while (timeout && (WiFi.status() != WL_CONNECTED)) {
        timeout--;
        if (debug) Serial.print(".");
        delay(1000);
      }
    }
    if (debug) Serial.println("Wifi connected");
  }
  if (!mqttClient.connected()) {
    while(!mqttClient.connected()){
      mqttClient.connect(broker, port);
      uint8_t timeout = 8;
      while (timeout && (!mqttClient.connected())){
        timeout--;
        if (debug) Serial.print(".");
        delay(1000);
      }
    }
    if (debug) Serial.println("MQTT Broker connected");
  }
}


void setup() {
  if (debug) {
    Serial.begin(115200);
    while (!Serial) {;}
    Serial.println("Start Program");
  }  
  
  WiFi.macAddress(mymac);
  for (byte i = 0; i < 6; ++i) {
    myMacAdr = myMacAdr + String(mymac[i], HEX);
    if (i < 5) myMacAdr = myMacAdr + '-';
  }
  if (debug) {Serial.println(myMacAdr);}

  wifiManager.setConfigPortalTimeout(180);
  if(!wifiManager.autoConnect("NodeMCU-1")) {
    if (debug) Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  } else {
    if (debug) Serial.println("Connection established");
  }

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  //GPS
  SerialGPS.begin(9600);
  while (!SerialGPS) {;}
  if (debug) Serial.println("GPS connected");
}


String myInput  = "", speedStr = "", latStr = "", lngStr ="";
void loop() {
  wifi_reconnect();
  mqttClient.poll();
  unsigned long currentMillis = millis();

  while (SerialGPS.available() > 0) {
      gps.encode(SerialGPS.read());
      
      if (gps.location.isUpdated()) {
        myInput  = "";
        StaticJsonDocument<2000> doc;
        
        doc["devId"] = myMacAdr;
        doc["latitude"] = gps.location.lat();
        doc["longitude"] = gps.location.lng();
        doc["speed"] = gps.speed.kmph();

        speedStr = String(doc["speed"]);
        latStr = String(doc["latitude"]);
        lngStr = String(doc["longitude"]);

        //yyyy-mm-dd hh:mm:ss
        char buffer[19];
        sprintf(buffer, "%d-%02d-%02d %02d:%02d:%02d", gps.date.year(), gps.date.month(),gps.date.day(),gps.time.hour()+2, gps.time.minute(), gps.time.second());
        doc["tstamp"] = String(buffer);        
        serializeJson(doc, myInput);
      }
  }

  
  if (currentMillis - previousMillis >= interval) {
    // save the last time a message was sent
    previousMillis = currentMillis;

    if (myInput=="") {
      Serial.println("No data");
    } else {
      if (debug) {
        Serial.print("Sending message to topic: ");
        Serial.print(topic_str + "/json");
        Serial.print(">>>>>>>>>>>> ");
        Serial.print(myInput);
        Serial.println("<<<<<");
      }
      mqttClient.beginMessage(topic_str + "/json");
      mqttClient.print(myInput);
      mqttClient.endMessage();
    
      //in addition plain values without json
      mqttClient.beginMessage(topic_str + "/"+ myMacAdr + "/value");
      mqttClient.print(speedStr);
      mqttClient.endMessage();

      mqttClient.beginMessage(topic_str + "/"+ myMacAdr + "/latitude");
      mqttClient.print(latStr);
      mqttClient.endMessage();
      
      mqttClient.beginMessage(topic_str + "/"+ myMacAdr + "/longitude");
      mqttClient.print(lngStr);
      mqttClient.endMessage();
    }
  }
 

}

