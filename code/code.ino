#include <Arduino.h>
#include "./src-gen/Statechart.cpp"
#include "./src-gen/Statechart.h"
#include "StatechartCallback.h"
#include "StatechartTimer.h"

Statechart statechart;
StatechartCallback callback;
StatechartTimer timerService;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Iniciando...");

  Wire.begin(21, 22);  // SDA e SCL do ESP32
  callback.beginDisplay();  // inicializa o display aqui, fora do construtor

  statechart.setOperationCallback(&callback);
  statechart.setTimerService(&timerService);
  statechart.enter();
}

void loop() {}
