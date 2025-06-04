#ifndef STATECHARTCALLBACK_H
#define STATECHARTCALLBACK_H

#include "./src-gen/Statechart.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// define o tamanho do display OLED e que nenhum pino de reset físico está sendo usado (-1)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

class StatechartCallback : public Statechart::OperationCallback {
public:
  Adafruit_SSD1306* display = nullptr;
  bool oledOK = false;

  StatechartCallback() {}

  // essa função deve ser chamada manualmente no setup() do .ino para inicializar o display OLED de forma segura (evita crash no construtor)
  void beginDisplay() {
    // cria o objeto display apenas se ainda não existir (evita múltiplas criações)
    if (display == nullptr) {
      display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    }

    // tenta inicializar o display no endereço I2C 0x3C (padrão), se falhar, exibe mensagem no Serial Monitor e marca oledOK = false
    if (!display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println("Falha ao inicializar o display!");
      oledOK = false;
      return;
    }

    // exibe uma mensagem de sucesso no display OLED
    oledOK = true;
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);
    display->println("Display OK!");
    display->display();
  }

  // mapeia os pinos para funcionamento do LED do ESP32
  void pinMode(sc_integer pin, sc_integer mode) override {
    if (mode == 1) {
      ::pinMode(pin, OUTPUT);
    } else {
      ::pinMode(pin, INPUT);
    }
  }

  // mapeia o LED para o real da placa
  void digitalWrite(sc_integer pin, sc_integer value) override {
    ::digitalWrite(pin, value);
  }

  // exemplo de função que poderá ser inicializada no estado
  void showStartup() override {
    Serial.println("Executando showStartup()");
  }

  // exemplo de função que poderá ser inicializada no estado
  void showRecipes() override {
    Serial.println("Executando showRecipes()");
  }

  // funções exigidas pela interface do itemis, mas ainda não possuem implementação funcional, mas precisa ser considerado pois já temos os estados implementados no itemis
  void shutdownSystem() override {}
  void initializeProcess() override {}
  void heat(sc_integer) override {}
  void time(sc_integer) override {}
  void showFinished() override {}
  void setTemperature(sc_integer) override {}
  void setTime(sc_integer) override {}
  void initializeSetupProcess() override {}

  // essa função foi criada no itemos com intuito de mostrar no Serial Monitor e agora Display o nome do estado atual, indicando que está no estado correto
  void showState(sc_string state) override {
    Serial.print("Estado atual: ");
    Serial.println(state);

    // verifica se o display foi inicializado corretamente com o que foi setado acima através do oledOK
    if (oledOK && display != nullptr) {
      display->clearDisplay();
      display->setCursor(0, 0);
      display->println("Estado atual:");
      display->println(state);
      display->display();
    }
  }
};

#endif // STATECHARTCALLBACK_H
