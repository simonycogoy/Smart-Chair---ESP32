
#ifndef MPU6050_H
#define MPU6050_H

#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#define MPU 0x68

void mpu_begin();

void mpu_calibrate(int leituras);

// Nova funcao: recalibra somente o giroscopio
void mpu_calibrateGyro(int leituras);

void mpu_loop();

void mpu_reset();

float getAngleX();
float getAngleY();
float getAngleZ();

int getDeltaZ(int target);

#endif
