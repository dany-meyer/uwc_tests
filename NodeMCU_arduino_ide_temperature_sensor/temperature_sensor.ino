const int temperaturePin = A0; //The middle pin in the sensor connects to A0
// Note : using LM35 temperature sensor 
void setup() {
  pinMode(temperaturePin, INPUT);
  Serial.begin(9600);
}

void loop() {
  int temp = analogRead(temperaturePin);
  float volts = (temp *500 )/ 1023;  // convert to celcius
  float celcius = volts; 
  Serial.println(celcius);
  delay(1000);
}

