
#include "mpu6050.h"

// ======================================================
// VARIAVEIS
// ======================================================

float AccX = 0;
float AccY = 0;
float AccZ = 0;

float GyroX = 0;
float GyroY = 0;
float GyroZ = 0;

float AccErrorX = 0;
float AccErrorY = 0;

float GyroErrorX = 0;
float GyroErrorY = 0;
float GyroErrorZ = 0;

float accAngleX = 0;
float accAngleY = 0;

float gyroAngleX = 0;
float gyroAngleY = 0;
float gyroAngleZ = 0;

float roll = 0;
float pitch = 0;
float yaw = 0;

float elapsedTime = 0;
float currentTime = 0;
float previousTime = 0;


// ======================================================
// INICIALIZA MPU6050
// ======================================================

void mpu_begin() {

  Wire.begin(25, 26);

  Wire.setClock(100000);

  Wire.beginTransmission(MPU);

  byte erro = Wire.endTransmission();

  if (erro == 0) {

    Serial.println("MPU6050 encontrado no endereco 0x68.");

  } else {

    Serial.println("ERRO: MPU6050 nao encontrado!");
    Serial.println("Verifique SDA, SCL, VCC e GND.");

    return;
  }

  // Acorda o sensor
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);

  // Configura acelerometro: +/- 2g
  Wire.beginTransmission(MPU);
  Wire.write(0x1C);
  Wire.write(0x00);
  Wire.endTransmission(true);

  // Configura giroscopio: +/- 250 graus/s
  Wire.beginTransmission(MPU);
  Wire.write(0x1B);
  Wire.write(0x00);
  Wire.endTransmission(true);

  delay(20);
}


// ======================================================
// LEITURA DO ACELEROMETRO
// ======================================================

void readAcel() {

  Wire.beginTransmission(MPU);

  Wire.write(0x3B);

  Wire.endTransmission(false);

  byte quantidade = Wire.requestFrom(MPU, 6, true);

  if (quantidade == 6) {

    int16_t rawX = (Wire.read() << 8) | Wire.read();
    int16_t rawY = (Wire.read() << 8) | Wire.read();
    int16_t rawZ = (Wire.read() << 8) | Wire.read();

    AccX = rawX / 16384.0f;
    AccY = rawY / 16384.0f;
    AccZ = rawZ / 16384.0f;
  }
}


// ======================================================
// LEITURA DO GIROSCOPIO
// ======================================================

void readGiro() {

  Wire.beginTransmission(MPU);

  Wire.write(0x43);

  Wire.endTransmission(false);

  byte quantidade = Wire.requestFrom(MPU, 6, true);

  if (quantidade == 6) {

    int16_t rawX = (Wire.read() << 8) | Wire.read();
    int16_t rawY = (Wire.read() << 8) | Wire.read();
    int16_t rawZ = (Wire.read() << 8) | Wire.read();

    GyroX = rawX / 131.0f;
    GyroY = rawY / 131.0f;
    GyroZ = rawZ / 131.0f;
  }
}


// ======================================================
// CALIBRACAO COMPLETA
// ======================================================

void mpu_calibrate(int leituras) {

  AccErrorX = 0;
  AccErrorY = 0;

  GyroErrorX = 0;
  GyroErrorY = 0;
  GyroErrorZ = 0;


  // ------------------------------------------
  // CALIBRACAO DO ACELEROMETRO
  // ------------------------------------------

  for (int nR = 0; nR < leituras; nR++) {

    readAcel();

    AccErrorX += (

      atan(
        AccY /
        sqrt(
          pow(AccX, 2) +
          pow(AccZ, 2)
        )
      ) * 180.0 / PI

    );

    AccErrorY += (

      atan(
        -1.0 * AccX /
        sqrt(
          pow(AccY, 2) +
          pow(AccZ, 2)
        )
      ) * 180.0 / PI

    );

    delay(2);
  }

  AccErrorX /= (float)leituras;
  AccErrorY /= (float)leituras;


  // ------------------------------------------
  // CALIBRACAO DO GIROSCOPIO
  // ------------------------------------------

  mpu_calibrateGyro(leituras);

}


// ======================================================
// CALIBRACAO SOMENTE DO GIROSCOPIO
// ======================================================

void mpu_calibrateGyro(int leituras) {

  GyroErrorX = 0;
  GyroErrorY = 0;
  GyroErrorZ = 0;

  for (int nR = 0; nR < leituras; nR++) {

    readGiro();

    GyroErrorX += GyroX;
    GyroErrorY += GyroY;
    GyroErrorZ += GyroZ;

    delay(2);
  }

  GyroErrorX /= (float)leituras;
  GyroErrorY /= (float)leituras;
  GyroErrorZ /= (float)leituras;

}


// ======================================================
// ATUALIZA OS ANGULOS
// ======================================================

void mpu_loop() {

  // ------------------------------------------
  // ACELEROMETRO
  // ------------------------------------------

  readAcel();

  accAngleX = (

    atan(
      AccY /
      sqrt(
        pow(AccX, 2) +
        pow(AccZ, 2)
      )
    ) * 180.0 / PI

  ) - AccErrorX;


  accAngleY = (

    atan(
      -1.0 * AccX /
      sqrt(
        pow(AccY, 2) +
        pow(AccZ, 2)
      )
    ) * 180.0 / PI

  ) - AccErrorY;


  // ------------------------------------------
  // CALCULA TEMPO ENTRE LEITURAS
  // ------------------------------------------

  previousTime = currentTime;

  currentTime = micros();

  elapsedTime =
    (currentTime - previousTime) / 1000000.0f;


  // ------------------------------------------
  // GIROSCOPIO
  // ------------------------------------------

  readGiro();

  GyroX -= GyroErrorX;
  GyroY -= GyroErrorY;
  GyroZ -= GyroErrorZ;


  // ------------------------------------------
  // INTEGRACAO DO GIRO
  // ------------------------------------------

  gyroAngleX += GyroX * elapsedTime;
  gyroAngleY += GyroY * elapsedTime;
  gyroAngleZ += GyroZ * elapsedTime;


  // ------------------------------------------
  // FILTRO COMPLEMENTAR
  // ------------------------------------------

  roll =
    0.96 * gyroAngleX +
    0.04 * accAngleX;

  pitch =
    0.96 * gyroAngleY +
    0.04 * accAngleY;


  // MPU6050 nao possui magnetometro.
  // YAW utiliza somente o giroscopio.

  yaw = gyroAngleZ;

}


// ======================================================
// RESET DOS ANGULOS
// ======================================================

void mpu_reset() {

  gyroAngleX = 0;
  gyroAngleY = 0;
  gyroAngleZ = 0;

  roll = 0;
  pitch = 0;
  yaw = 0;

  currentTime = micros();
  previousTime = currentTime;

  mpu_loop();

}


// ======================================================
// RETORNA ROLL
// ======================================================

float getAngleX() {

  return roll;

}


// ======================================================
// RETORNA PITCH
// ======================================================

float getAngleY() {

  return pitch;

}


// ======================================================
// RETORNA YAW
// ======================================================

float getAngleZ() {

  return yaw;

}


// ======================================================
// RETORNA DIFERENCA PARA UM ANGULO
// ======================================================

int getDeltaZ(int target) {

  if (target < 0) {

    return round(
      getAngleZ() - target
    );

  } else {

    return round(
      target - getAngleZ()
    );

  }

}
