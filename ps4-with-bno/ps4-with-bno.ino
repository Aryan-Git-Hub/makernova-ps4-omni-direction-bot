#include <PS4Controller.h>
#include <Wire.h>
#include <math.h>
#include <Wire.h>
#include "Adafruit_BNO08x.h"
#define PI 3.14159265359

// BNO
Adafruit_BNO08x bno08x(-1);  // -1 = no reset pin used
// ESP32 defalt light pin
const int light = 2;
// motor pins
const int motor1 = 12;
const int motor2 = 23;
const int motor3 = 33;
// motor direction pins
const int motor1_dir = 13;
const int motor2_dir = 15;
const int motor3_dir = 32;

// important varialbes
float k = 0.7f;                                // speed factor
int w1 = 0, w2 = 0, w3 = 0;                    // angular velocity of wheels
int w1_dir = LOW, w2_dir = LOW, w3_dir = LOW;  // direction of each wheels
int delay_on_click = 200;                      // delay after clicking on buttons
int maxspeed = 200;                            // maxspeed to while we controlling bot by left arrows of ps4
bool bno_rotation = false;                     // to function bno_rotation with the trigger button
float yaw = 0;

void setup() {
  Serial.begin(115200);

  // BNO setup
  Wire.begin(21, 22);  // SDA=21, SCL=22 on ESP32
  if (!bno08x.begin_I2C()) {
    Serial.println("Failed to find BNO085 over I2C!");
    while (1);
  }
  Serial.println("BNO085 found over I2C");
  if (!bno08x.enableReport(SH2_ROTATION_VECTOR, 10000)) {
    Serial.println("Could not enable rotation vector");
  }

  // PS4 setup
  PS4.begin();
  Serial.println("Ready.");
  pinMode(light, OUTPUT);
  pinMode(motor1, OUTPUT);
  pinMode(motor2, OUTPUT);
  pinMode(motor3, OUTPUT);
  pinMode(motor1_dir, OUTPUT);
  pinMode(motor2_dir, OUTPUT);
  pinMode(motor3_dir, OUTPUT);
}


void func(int x, int y, int w, int &w1, int &w2, int &w3, int &w1_dir, int &w2_dir, int &w3_dir) {
  float fw1 = 1.155f * y + w / 3.0f;
  float fw2 = -x - 0.577f * y + w / 3.0f;
  float fw3 = x - 0.577f * y + w / 3.0f;

  // setting up directions of wheel
  if (fw1 < 0) {
    fw1 = -fw1;
    w1_dir = LOW;
  } else w1_dir = HIGH;
  if (fw2 < 0) {
    fw2 = -fw2;
    w2_dir = LOW;
  } else w2_dir = HIGH;
  if (fw3 < 0) {
    fw3 = -fw3;
    w3_dir = LOW;
  } else w3_dir = HIGH;

  // changing speed by the factor of k
  fw1 = fw1 * k;
  fw2 = fw2 * k;
  fw3 = fw3 * k;

  w1 = (int)fw1;
  w2 = (int)fw2;
  w3 = (int)fw3;
}


void loop() {
  // BNO
  sh2_SensorValue_t sensorValue;
  if (bno08x.getSensorEvent(&sensorValue)) {
    if (sensorValue.sensorId == SH2_ROTATION_VECTOR) {
      float qw = sensorValue.un.rotationVector.real;
      float qx = sensorValue.un.rotationVector.i;
      float qy = sensorValue.un.rotationVector.j;
      float qz = sensorValue.un.rotationVector.k;

      float ysqr = qy * qy;
      float t3 = +2.0f * (qw * qz + qx * qy);
      float t4 = +1.0f - 2.0f * (ysqr + qz * qz);
      yaw = atan2(t3, t4);
      Serial.print("Yaw: ");   Serial.print(yaw);
    }
  }
  if (PS4.isConnected()) {
    // PS4 connected feedback
    digitalWrite(light, HIGH);

    // rotation of bot by trigger values;
    int L2_val = PS4.L2Value();
    int R2_val = PS4.R2Value();
    int w = L2_val - R2_val;

    int x, y;

    if (PS4.Triangle()) {
      yaw = 0;
      bno_rotation = bno_rotation?false:true;
      delay(delay_on_click);
    }
    // controlling bot by arrow buttons
    if (PS4.Up() || PS4.Down() || PS4.Left() || PS4.Right()) {
      int a = 0;
      int b = 0;
      if (PS4.Up()) a = 1;
      else if (PS4.Down()) a = -1;
      if (PS4.Left()) b = 1;
      else if (PS4.Right()) b = -1;
      x = maxspeed*a;
      y = maxspeed*b;
    } else {
      x = PS4.LStickY();
      y = -PS4.LStickX();
    }


    // speed controlling
    if (PS4.L1() && k > 0.3f) {
      k -= 0.1f;
      delay(delay_on_click);
    } else if (PS4.R1() && k < 0.9f) {
      k += 0.1f;
      delay(delay_on_click);
    }

    if (abs(x) < 10) x = 0;
    if (abs(y) < 10) y = 0;
    if (abs(w) < 10) w = 0;


    if (bno_rotation) {
      int temp_x = x, temp_y = y;
      x = (int)(temp_x*cos(yaw)+temp_y*sin(yaw));
      y = (int)(-temp_x*sin(yaw)+temp_y*cos(yaw));
    }
    func(x, y, w, w1, w2, w3, w1_dir, w2_dir, w3_dir);
    analogWrite(motor1, w1);
    analogWrite(motor2, w2);
    analogWrite(motor3, w3);
    digitalWrite(motor1_dir, w1_dir);
    digitalWrite(motor2_dir, w2_dir);
    digitalWrite(motor3_dir, w3_dir);
    Serial.printf("yaw = %f, w1 = (%d, %d), w2 = (%d, %d), w3 = (%d, %d)\n", yaw, w1, w1_dir, w2, w2_dir, w3, w3_dir);
  } else {
    digitalWrite(light, HIGH);
    delay(1000);
    digitalWrite(light, LOW);
    delay(1000);
  }
}