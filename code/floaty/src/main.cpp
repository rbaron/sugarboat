#include <Adafruit_TinyUSB.h>

#include "Wire.h"
#include "floaty/mpu6050.h"

floaty::IMU imu;

void setup() {
  Wire.begin();
  Wire.setClock(400000);

  Serial.begin(115200);
  while (!Serial)
    ;

  if (imu.Init()) {
    Serial.printf("[main] Error initializing imu");
  } else {
    Serial.println("OK!!");
  }
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  float angle = imu.GetTilt();
  Serial.printf("Got angle: %.2f\n", angle);
  delay(100);
}