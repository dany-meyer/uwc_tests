#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <time.h>
#include <TZ.h>
#include <FS.h>
#include <LittleFS.h>// Needed to upload mqtt cert files to the microporocessor
#include <CertStoreBearSSL.h>
#include <TinyGPS++.h>
#include <ArduinoJson.h>
#include <string.h>
#include <SoftwareSerial.h>

// Update these with values suitable for your network.
// Note: Go through the readme file before attempting this code.
const char* mqtt_server = "<YOUR_PRIVATE_CLUSTER_HIVEMQ_ID>";

// A single, global CertStore which can be used by all connections.
// Needs to stay live the entire time any of the WiFiClientBearSSLs
// are present.
BearSSL::CertStore certStore;
WiFiClientSecure espClient;

bool debug = true;

static byte mymac[] = { 0x00,0x00,0x00,0x00,0x00,0x00 };
String myMacAdr = "";
float latitude , longitude;
String date_str , time_str , lat_str , lng_str;
PubSubClient * client;
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (500)
char msg[MSG_BUFFER_SIZE];
int value = 0;
int temperaturePin = A0;

WiFiManager wifiManager;
WiFiClient wifiClient;

//setup gps
TinyGPSPlus gps;
// TX --> D1, RX --> D2
// see the pinout for the NodeMCU 12 to find the mapping between the GPIO's and the pins on the board:
// https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

SoftwareSerial SerialGPS(5,4);



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
}

void setDateTime() {
  // You can use your own timezone, but the exact time is not used at all.
  // Only the date is needed for validating the certificates.
  configTime(TZ_Europe_Berlin, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(100);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.printf("%s %s", tzname[0], asctime(&timeinfo));
}
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if the first character is present
  if ((char)payload[0] != NULL) {
    digitalWrite(LED_BUILTIN, LOW); // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
  } else {
    digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
  }
}
void mqtt_connect(){
   // Loop until we’re reconnected
  while (!client->connected()) {
    Serial.print("Attempting MQTT connection…");
    String clientId = "ESP8266Client - MyClient";
    // Attempt to connect
    // Insert your password
    if (client->connect(clientId.c_str(), "CLIENT_USERNAME", "<CLIENT_PASSWORD>")) {
      Serial.println("connected");
      // Once connected, publish an announcement…
      client->publish("testTopics", "hello world");
      // … and resubscribe
      client->subscribe("testTopic");
    } else {
      Serial.print("failed, rc = ");
      Serial.print(client->state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
    yield(); 
  }
  }

void setup() {
  if (debug) {
    Serial.begin(115200);
    pinMode(temperaturePin,INPUT);
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
  if(!wifiManager.autoConnect("Genesis")) {
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
  LittleFS.begin();
  setDateTime();

  int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
  Serial.printf("Number of CA certs read: %d\n", numCerts);

  if(numCerts==0){
    Serial.printf("No certs Found, remeber to upload cert file");
    }
  BearSSL:: WiFiClientSecure *bear = new BearSSL:: WiFiClientSecure();
  //intergrate the cert store with this connection
  bear->setCertStore(&certStore);
  client = new PubSubClient(*bear);

  client-> setServer(mqtt_server,8883);
  client->setCallback(callback);

  //GPS
  SerialGPS.begin(9600);
  while (!SerialGPS) {;}
  if (debug) Serial.println("GPS connected");
}

String myInput  = "", speedStr = "", latStr = "", lngStr ="";
void loop() {
  wifi_reconnect();

  if(!client->connected()){
     mqtt_connect();
    }
   client->loop(); 
   unsigned long now = millis();

   while(SerialGPS.available() > 0){
    if(gps.encode(SerialGPS.read())){
       if (gps.location.isValid() && gps.date.isValid()) {
        
        myInput  = "";
        date_str = String(gps.date.year());
        latitude = gps.location.lat();
        lat_str = String(latitude , 6); // latitude location is stored in a string
        longitude = gps.location.lng();
        lng_str = String(longitude , 6);
        int temp = analogRead(temperaturePin);
        float volts = (temp *500 )/ 1023;  
        float celsius = volts;

        String jsonStr = "{";
        jsonStr += "\"devId\":\"" + myMacAdr + "\",";
        jsonStr += "\"latitude\":" + String(gps.location.lat(), 6) + ",";
        jsonStr += "\"longitude\":" + String(gps.location.lng(), 6) + ",";
        jsonStr += "\"speed\":" + String(gps.speed.kmph()) + ",";
        jsonStr += "\"temp\":"+ String(celsius);
  
        jsonStr += "}";
        Serial.println(jsonStr);

        const char* jsonChar = jsonStr.c_str();
        const char* topic=myMacAdr.c_str();

        Serial.println(jsonStr);
        client->publish(topic, jsonChar);
        delay(5000);
      }else{
        Serial.println("No data!");
        delay(5000);
      }
      
      }
    
    }
}
