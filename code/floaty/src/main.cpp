#include <Adafruit_TinyUSB.h>
#include <Arduino.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(11520);

  Serial.println("Hello, world");
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("loop");
  delay(1000);
}