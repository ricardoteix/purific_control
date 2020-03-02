/*
  Controle de Fluxo de Água com Arduino

  Este projeto foi desenvolvido com o propósito de controlar o fluxo de água de um purificador.
  Toda a descrição do projeto do hardware e hidráulico foi apresentada em 
  https://www.ricardoteix.com/controle-de-fluxo-de-agua-com-arduino/.

  Todo o projeto, incluíndo e hardware e software, é livre para ser utilizado e/ou modificado 
  para uso pessoal ou comercial com a única exigência de referenciar a fonte original como abaixo.
  
  Baseado em https://www.ricardoteix.com/controle-de-fluxo-de-agua-com-arduino/
  
*/

// https://github.com/avishorp/TM1637
#include <TM1637Display.h>

// https://github.com/mathertel/RotaryEncoder
#include <RotaryEncoder.h>

#define ESPERA          0
#define LIBERA_CONTA    1
#define ATUALIZA        2

const int CLK = 4; //Set the CLK pin connection to the display
const int DIO = 3; //Set the DIO pin connection to the display

TM1637Display display(CLK, DIO);

const byte ADJ = A0; 
const byte BZR = 12;
const byte SW = 6;
const byte SOLENOIDE = 5;

const byte encoderPinA = 8;
const byte encoderPinB = 9;

RotaryEncoder encoder(encoderPinA, encoderPinB);

int currentMls = 0;
const byte LED = 13;

// Flow Sensor
byte flowSensorInterrupt = 0;  // 0 = digital pin 2
byte flowSensorPin       = 2;
volatile byte flowPulseCount;
float calibrationFactor = 4.5;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long oldTime;
// Flow Sensor End

int adj;
bool freeFlow = false;
int currentState = ESPERA;

void setup() {  
  Serial.begin(9600);
  
  pinMode(LED, OUTPUT);
  pinMode(BZR, OUTPUT);
  pinMode(SOLENOIDE, OUTPUT);
  pinMode(ADJ, INPUT);
  pinMode(SW, INPUT);
  
  display.setBrightness(0x0f);
  display.showNumberDec(0);

  initFlowSensor();
  
  Serial.println("INICIALIZA");
  Serial.println("ESTADO -> ESPERA");

  
}

void loop() {

  switch (currentState) {
    case ESPERA: {
      executarEspera();
      break;
    }
    
    case LIBERA_CONTA: {
      executarLiberaConta();
      break;
    }    
    
  }  
  
}

void executarEspera() {
  adj = analogRead(ADJ);
  int pressed = digitalRead(SW);
  if (!pressed) {
      while(digitalRead(SW) == 0) {
        if (currentMls == 0) {
          freeFlow = true;
          checkFlow();
          digitalWrite (SOLENOIDE, HIGH);
          display.showNumberDec(totalMilliLitres);
        }
      }
      Serial.println("ESTADO -> LIBERA_CONTA");
      digitalWrite (SOLENOIDE, LOW);
      currentState = LIBERA_CONTA;
  }
  runEncoder();
}

void executarLiberaConta() {
  unsigned int delta = currentMls - totalMilliLitres;
  digitalWrite (SOLENOIDE, HIGH);
  Serial.println("LIBERA SOLENOIDE");
  while (currentMls > totalMilliLitres) {
    checkFlow();

    int parcial = currentMls - totalMilliLitres;
    if (parcial < 0) {
      parcial = 0;
    }
    display.showNumberDec(parcial);

    int pressed = digitalRead(SW);
    if (!pressed) {
        while(digitalRead(SW) == 0) {}
        currentMls = 0;
    }
  } 
  
  encoder.setPosition(0);
  currentMls = 0;
  totalMilliLitres = 0;
  display.showNumberDec(0);
  
  digitalWrite (SOLENOIDE, LOW);

  if (!freeFlow) {
    tone(BZR, 300, 500);
    delay(1000);
    tone(BZR, 300, 500);
    delay(1000);
    tone(BZR, 300, 500);
    delay(1000);
  }
  
  Serial.println("ESTADO -> ESPERA");
  freeFlow = false;
  currentState = ESPERA;
}

void runEncoder() {
  
  static int encoderPosition = 0;  
  
  encoder.tick();  
  int newPos = encoder.getPosition();
  
  if (encoderPosition != newPos) {    
    encoderPosition = newPos;
    if (encoderPosition * -1 >= 0) {
      currentMls = -1 * encoderPosition * 10;
    } else {
      encoder.setPosition(0);
    }    
    display.showNumberDec(currentMls);
  }
}


void initFlowSensor () {  
  pinMode(flowSensorPin, INPUT);
  digitalWrite(flowSensorPin, HIGH);  
  
  flowPulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  oldTime = 0;
  
  attachInterrupt(flowSensorInterrupt, flowPinInsterrupt, FALLING);
}

void checkFlow() {
    
  if((millis() - oldTime) > 1000) {
    
    detachInterrupt(flowSensorInterrupt);
    
    float adjPerc = ((float)adj / 600.0 * (calibrationFactor - 0.5)) + 0.5;
    flowRate = ((1000.0 / (millis() - oldTime)) * flowPulseCount) / adjPerc;
    oldTime = millis();
    flowMilliLitres = ( flowRate / 60 ) * 1000;
    totalMilliLitres += flowMilliLitres;
    
    flowPulseCount = 0;
    attachInterrupt(flowSensorInterrupt, flowPinInsterrupt, FALLING);
    
  }
}

void flowPinInsterrupt() {
  // Increment the pulse counter
  flowPulseCount++;
  Serial.print("  FLOW -> ");
  Serial.println(flowPulseCount);
}
