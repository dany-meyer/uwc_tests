#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>
#include <string.h>
#include <SoftwareSerial.h>
#include <DHT.h>
#include <ArduinoMqttClient.h>

#define DHTPin 5
#define DHTType DHT22 
bool debug = true;

DHT dht(DHTPin, DHTType);

WiFiManager wifiManager;
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "broker.hivemq.com"; //"test.mosquitto.org";
int        port     = 1883;
String topic_str = "uwc_test/environment";

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

  dht.begin();

}


String myInput  = "";
void loop() {
  wifi_reconnect();
  mqttClient.poll();
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time a message was sent
    previousMillis = currentMillis;

    float newT = dht.readTemperature();
    float newH = dht.readHumidity();
    Serial.println(newT);


    myInput  = "";
    StaticJsonDocument<2000> doc;
        
    doc["devId"] = myMacAdr;
    doc["temp"] = newT;
    doc["hum"] = newH;
    
    String tempStr = String(doc["temp"]);
    String humStr = String(doc["hum"]);
    
    //yyyy-mm-dd hh:mm:ss
    //char buffer[19];
    //sprintf(buffer, "%d-%02d-%02d %02d:%02d:%02d", gps.date.year(), gps.date.month(),gps.date.day(),gps.time.hour()+2, gps.time.minute(), gps.time.second());
    //doc["tstamp"] = String(buffer);        
    
    serializeJson(doc, myInput);
    

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
    mqttClient.beginMessage(topic_str + "/"+ myMacAdr + "/temp");
    mqttClient.print(tempStr);
    mqttClient.endMessage();

    mqttClient.beginMessage(topic_str + "/"+ myMacAdr + "/hum");
    mqttClient.print(humStr);
    mqttClient.endMessage();
  
  }
 

}

