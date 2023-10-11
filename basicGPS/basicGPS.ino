#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoMqttClient.h>

// Use Arduino UNO pin 4 and 5
// https://lastminuteengineers.com/neo6m-gps-arduino-tutorial/
// Choose two Arduino pins to use for software serial

// Node MCU:
// D1 --> pin 5
// D2 --> pin 4
int RXPin = 4;
int TXPin = 5;
int GPSBaud = 9600;

// Create a TinyGPS++ object
TinyGPSPlus gps;

// Create a software serial port called "gpsSerial"
SoftwareSerial gpsSerial(TXPin, RXPin);
bool debug = true;

void setup()
{
  if (debug) {
    Serial.begin(9600);
    while (!Serial) {;}
    Serial.println("Start Program");
  } 
  // Start the software serial port at the GPS's default baud
  gpsSerial.begin(GPSBaud);
}

const long interval = 5000;
unsigned long previousMillis = 0;


void loop()
{
  while (gpsSerial.available() > 0)
    if (gps.encode(gpsSerial.read()))
      delay(10);

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    displayInfo();
  }
}

void displayInfo()
{
  if (gps.location.isValid())
  {
    Serial.print("Latitude: ");
    Serial.print(gps.location.lat(), 6);
    Serial.print("   Longitude: ");
    Serial.println(gps.location.lng(), 6);
    //Serial.print("Altitude: ");
    //Serial.println(gps.altitude.meters());
  }
  else
  {
    Serial.println("Location: Not Available");
  }
  
  Serial.print("Date: ");
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print("/");
    Serial.print(gps.date.day());
    Serial.print("/");
    Serial.println(gps.date.year());
  }
  else
  {
    Serial.println("Not Available");
  }

  Serial.print("Time: ");
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(":");
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(":");
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(".");
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.println(gps.time.centisecond());
  }
  else
  {
    Serial.println("Not Available");
  }

  Serial.println();
  Serial.println();
}