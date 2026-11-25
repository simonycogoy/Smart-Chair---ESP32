
/*
  SMART CHAIR - CONTROLE DE GIRO
  ESP32 DevKit V1 + MPU6050

  SDA -> GPIO 25
  SCL -> GPIO 26
  Endereco I2C: 0x68
*/

#include "mpu6050.h"

// ====================================================
// LIMITES DOS MOVIMENTOS
// ====================================================

// Eixo Z - esquerda e direita
const float LIMITE_ESQUERDA = 40.0;
const float LIMITE_DIREITA  = -40.0;

// Eixo X - frente e tras
const float LIMITE_FRENTE = -30.0;
const float LIMITE_TRAS   = 40.0;

// Angulo desejado para cada giro
const float ANGULO_ALVO = 40.0;
const float ANGULO_ALVOMenos = -40.0;

// Tempo aguardando a cadeira parar
const unsigned long TEMPO_PARADA = 500;


// ====================================================
// ESTADOS DO SISTEMA
// ====================================================

enum Estado {

  AGUARDANDO,
  GIRANDO_ESQUERDA,
  GIRANDO_DIREITA,
  RECALIBRANDO

};

Estado estado = AGUARDANDO;


// ====================================================
// VARIAVEIS DO GIRO
// ====================================================

float anguloInicial = 0;
float diferencaAngulo = 0;

unsigned long inicioParada = 0;


// ====================================================
// SETUP
// ====================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("====================================");
  Serial.println("       SMART CHAIR - MPU6050");
  Serial.println("====================================");

  // Inicializa o sensor
  mpu_begin();

  Serial.println("MPU6050 inicializado.");

  // Calibracao inicial
  Serial.println();
  Serial.println("Calibrando sensor...");
  Serial.println("Mantenha o MPU parado!");

  delay(2000);

  mpu_calibrate(200);

  // Zera os angulos
  mpu_reset();

  Serial.println();
  Serial.println("SENSOR CALIBRADO!");
  Serial.println("Sistema aguardando comandos.");
  Serial.println();

}


// ====================================================
// LOOP PRINCIPAL
// ====================================================

void loop() {

  // Atualiza os angulos somente fora da recalibracao
  if (estado != RECALIBRANDO) {
    mpu_loop();
  }

  float roll = getAngleX();
  float yaw = getAngleZ();


  // ==================================================
  // MAQUINA DE ESTADOS
  // ==================================================

  switch (estado) {


    // ================================================
    // AGUARDANDO UM NOVO COMANDO
    // ================================================

    case AGUARDANDO:

      diferencaAngulo = 0;

      // GIRO PARA ESQUERDA
      if (yaw > LIMITE_ESQUERDA) {

        anguloInicial = yaw;

        estado = GIRANDO_ESQUERDA;

        Serial.println();
        Serial.println("COMANDO,ESQUERDA,90");

        Serial.println("Iniciando giro para esquerda.");

      }

      // GIRO PARA DIREITA
      else if (yaw < LIMITE_DIREITA) {

        anguloInicial = yaw;

        estado = GIRANDO_DIREITA;

        Serial.println();
        Serial.println("COMANDO,DIREITA,90");

        Serial.println("Iniciando giro para direita.");

      }

      // MOVIMENTO PARA FRENTE
      else if (roll < LIMITE_FRENTE) {

        Serial.println("MOVIMENTO,FRENTE");

      }

      // MOVIMENTO PARA TRAS
      else if (roll > LIMITE_TRAS) {

        Serial.println("MOVIMENTO,TRAS");

      }

      // PARADO
      else {

        Serial.println("MOVIMENTO,PARADO");

      }

      break;


    // ================================================
    // GIRO PARA ESQUERDA
    // ================================================

    case GIRANDO_ESQUERDA:

      // Calcula quanto o sensor girou
      // desde o inicio da manobra

      diferencaAngulo = yaw - anguloInicial;

      Serial.print("GIRANDO ESQUERDA | ANGULO: ");

      Serial.println(diferencaAngulo, 2);

      // Verifica se atingiu 90 graus
      if (diferencaAngulo >= ANGULO_ALVO || diferencaAngulo <= ANGULO_ALVOMenos ) {

        Serial.println("COMANDO,PARAR");

        Serial.println("Giro de 90 graus concluido.");

        inicioParada = millis();

        estado = RECALIBRANDO;

      }

      break;


    // ================================================
    // GIRO PARA DIREITA
    // ================================================

    case GIRANDO_DIREITA:

      // Calcula quanto o sensor girou
      // desde o inicio da manobra

      diferencaAngulo = anguloInicial - yaw;

      Serial.print("GIRANDO DIREITA | ANGULO: ");

      Serial.println(diferencaAngulo, 2);

      // Verifica se atingiu 90 graus
      if (diferencaAngulo >= ANGULO_ALVO) {

        Serial.println("COMANDO,PARAR");

        Serial.println("Giro de 90 graus concluido.");

        inicioParada = millis();

        estado = RECALIBRANDO;

      }

      break;


    // ================================================
    // RECALIBRACAO DO SENSOR
    // ================================================

    case RECALIBRANDO:

      // Aguarda a cadeira parar completamente
      if (millis() - inicioParada >= TEMPO_PARADA) {

        Serial.println();
        Serial.println("Iniciando recalibracao...");

        // Recalibra somente o giroscopio
        mpu_calibrateGyro(200);

        // Zera a referencia angular
        mpu_reset();

        anguloInicial = 0;
        diferencaAngulo = 0;

        Serial.println("Recalibracao concluida!");

        Serial.println("Novo comando liberado.");

        estado = AGUARDANDO;

      }

      break;

  }


  delay(20);

}
