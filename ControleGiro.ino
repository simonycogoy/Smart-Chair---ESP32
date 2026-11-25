
/*
  SMART CHAIR - CONTROLE DE GIRO
  ESP32 DevKit V1 + MPU6050

  SDA -> GPIO 25
  SCL -> GPIO 26
  Endereco I2C: 0x68

  Comunicacao:
  ESP32 -> Bluetooth Classico -> Raspberry Pi
  Nome Bluetooth: SmartChair-ESP32
*/

#include "mpu6050.h"
#include <BluetoothSerial.h>

// Objeto de comunicacao Bluetooth
BluetoothSerial SerialBT;


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
// COMUNICACAO BLUETOOTH
// ====================================================

// Guarda o ultimo comando enviado para evitar repeticoes
String ultimoComandoEnviado = "";

void enviarComando(String comando) {

  // Evita enviar repetidamente o mesmo comando
  if (comando == ultimoComandoEnviado) {
    return;
  }

  // Mostra o comando no Monitor Serial
  Serial.println(comando);

  // Envia o comando para o Raspberry Pi
  if (SerialBT.hasClient()) {
    SerialBT.println(comando);
  }

  // Atualiza o ultimo comando enviado
  ultimoComandoEnviado = comando;

}


// ====================================================
// SETUP
// ====================================================

void setup() {

  Serial.begin(115200);

  delay(1000);


  // ==================================================
  // INICIALIZACAO DO BLUETOOTH
  // ==================================================

  if (SerialBT.begin("SmartChair-ESP32")) {

    Serial.println("Bluetooth iniciado!");
    Serial.println("Nome: SmartChair-ESP32");

  } else {

    Serial.println("Erro ao iniciar Bluetooth!");

  }


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

        enviarComando("COMANDO,ESQUERDA,90");

        Serial.println("Iniciando giro para esquerda.");

      }


      // GIRO PARA DIREITA
      else if (yaw < LIMITE_DIREITA) {

        anguloInicial = yaw;

        estado = GIRANDO_DIREITA;

        enviarComando("COMANDO,DIREITA,90");

        Serial.println("Iniciando giro para direita.");

      }


      // MOVIMENTO PARA FRENTE
      else if (roll < LIMITE_FRENTE) {

        enviarComando("MOVIMENTO,FRENTE");

      }


      // MOVIMENTO PARA TRAS
      else if (roll > LIMITE_TRAS) {

        enviarComando("MOVIMENTO,TRAS");

      }


      // PARADO
      else {

        enviarComando("MOVIMENTO,PARADO");

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


      // Verifica se atingiu o limite positivo ou negativo
      if (diferencaAngulo >= ANGULO_ALVO ||
          diferencaAngulo <= ANGULO_ALVOMenos) {

        enviarComando("COMANDO,PARAR");

        Serial.println("Giro concluido.");

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


      // Verifica se atingiu o angulo desejado
      if (diferencaAngulo >= ANGULO_ALVO) {

        enviarComando("COMANDO,PARAR");

        Serial.println("Giro concluido.");

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

        // Libera o envio de um novo comando
        ultimoComandoEnviado = "";

        estado = AGUARDANDO;

      }

      break;

  }


  delay(20);

}
