#ifndef STATECHARTCALLBACK_H
#define STATECHARTCALLBACK_H

#include "./src-gen/Statechart.h"
#include <Arduino.h>
// DisplayOLED
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// Matriz teclado 4x4
#include <Keypad.h>

// Define o tamanho do display OLED e que nenhum pino de reset físico está sendo usado (-1)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// Mapeamento do teclado 4x4
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27};  // R1 R2 R3 R4
byte colPins[COLS] = {26, 25, 33, 32};  // C1 C2 C3 C4

class StatechartCallback : public Statechart::OperationCallback {
public:
  Adafruit_SSD1306* display = nullptr;
  bool oledOK = false;

  Keypad* keypad = nullptr; // Mover Keypad para public para acessibilidade
  bool matrixOK = false;

  String inputBuffer = ""; // Buffer para armazenar a entrada do teclado
  unsigned long lastKeyPressTime = 0; // Para controle de tempo de digitação

  StatechartCallback() {}

  // Novo método para definir o ponteiro da máquina de estados
  void setStatechart(Statechart* sc) {
    myStatechart = sc;
  }

  // Configuração inicial do display
  void beginDisplay() override {
    Serial.println("Iniciando display via callback");

    if (display == nullptr) {
      display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    }

    if (!display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println("Falha ao inicializar o display!");
      oledOK = false;
      return;
    }

    oledOK = true;
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);
    display->println("Display OK!");
    display->display();
  }

  // Mapeia os pinos para funcionamento do LED do ESP32
  void pinMode(sc_integer pin, sc_integer mode) override {
    if (mode == 1) {
      ::pinMode(pin, OUTPUT);
    } else {
      ::pinMode(pin, INPUT);
    }
  }

  // Mapeia o LED para o real da placa
  void digitalWrite(sc_integer pin, sc_integer value) override {
    ::digitalWrite(pin, value);
  }

  // Exemplo de função que poderá ser inicializada no estado
  void showStartup() override {
    Serial.println("Executando showStartup()");
  }

  // Exemplo de função que poderá ser inicializada no estado
  void showRecipes() override {
    Serial.println("Executando showRecipes()");
    // Ao entrar no menu de receitas, limpa o buffer do teclado
    inputBuffer = ""; 
    // Chama showMenu com a opção para limpar a tela
    showMenu(true); 
  }
  
  // Funções para exibir o menu e a entrada do teclado
   void showMenu(bool clearScreen = true) {
    if (oledOK && display != nullptr) {
      if (clearScreen) {
        display->clearDisplay();
      }
      display->setCursor(0, 0);
      display->println("Bem-vindo!");
      display->setCursor(0, 16);
      display->println("1 - Iniciar");
      display->println("2 - Sair");
      display->println("Pressione A");
      
      // Exibir o que foi digitado logo abaixo, em uma nova linha
      display->setCursor(0, 48);
      display->print("Digitado: ");
      display->println(inputBuffer); // Mostra o buffer de entrada
      display->display();
    }
  }

  // Funções exigidas pela interface do itemis, mas ainda não possuem implementação funcional, mas precisa ser considerado pois já temos os estados implementados no itemis
  void shutdownSystem() override {}
  void initializeProcess() override {}
  void heat(sc_integer) override {}
  void time(sc_integer) override {}
  void showFinished() override {}
  void setTemperature(sc_integer) override {}
  void setTime(sc_integer) override {}
  void initializeSetupProcess() override {}

  // Essa função foi criada no itemos com intuito de mostrar no Serial Monitor e agora Display o nome do estado atual, indicando que está no estado correto
  void showState(sc_string state) override {
    Serial.print("Estado atual: ");
    Serial.println(state);

    // Verifica se o display foi inicializado corretamente com o que foi setado acima através do oledOK
    if (oledOK && display != nullptr) {
      display->clearDisplay();
      display->setCursor(0, 0);
      display->println("Estado atual:");
      display->println(state);
      display->display();
    }
  }

  // Configuração da screen
  void showIdleScreen() override {
    Serial.println("Exibindo tela de IDLE");

    if (oledOK && display != nullptr) {
      display->clearDisplay();
      display->setCursor(0, 0);
      display->println("Bem-vindo!");
      display->setCursor(0, 16);
      display->println("1 - Iniciar");
      display->println("2 - Sair");
      display->println("Pressione A");
      display->display();
    }
  }

  void beginMatrix() override {
    Serial.println("Iniciando teclado matricial via callback");

    if (keypad == nullptr) {
      keypad = new Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
    }

    if (keypad != nullptr) {
      matrixOK = true;
      Serial.println("Teclado iniciado com sucesso.");
    } else {
      Serial.println("Falha ao iniciar teclado.");
      matrixOK = false;
    }
  }

  // Modificar readKeypadAndDisplay para disparar eventos
  void readKeypadAndDisplay() {
    if (matrixOK && keypad != nullptr) {
      char key = keypad->getKey();

      if (key) {
        Serial.print("Tecla Pressionada: ");
        Serial.println(key);
        
        // Adiciona a tecla ao buffer
        inputBuffer += key;
        lastKeyPressTime = millis();
        
        // Atualiza o display com o menu e a entrada digitada
        showMenu(false); 

        // !!! DISPARA OS EVENTOS PARA O STATECHART AQUI !!!
        if (myStatechart != nullptr) {
          if (key == '1') {
            myStatechart->raiseStart_button();
            inputBuffer = ""; // Limpa o buffer após o evento ser enviado
          } else if (key == '2') {
            myStatechart->raiseExit_process();
            inputBuffer = ""; // Limpa o buffer
          } else if (key == 'A') {
            // Se 'A' for para o menu principal, ou outra ação
            myStatechart->raiseMenu(); 
            inputBuffer = ""; // Limpa o buffer
          }
        }
      }
    }
    // Para limpar o buffer após um tempo sem digitação
    if (inputBuffer.length() > 0 && millis() - lastKeyPressTime > 3000) { // 3 segundos
      inputBuffer = "";
      showMenu(true); // Limpa e redesenha a tela para remover o buffer antigo
    }
  }

  private:
    Statechart* myStatechart = nullptr; // <--- Esta linha estava faltando! Importante eu deiar ela aqui para puxar o cycle

};



#endif // STATECHARTCALLBACK_H