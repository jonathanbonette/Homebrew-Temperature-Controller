#ifndef STATECHARTCALLBACK_H
#define STATECHARTCALLBACK_H

#include "src-gen/Statechart.h"
#include <Arduino.h>
// displayOLED
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// matrix4x4
#include <Keypad.h>

// Incluir FreeRTOS Headers
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h" // Para filas

// --- DEFINES DO HARDWARE ---
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
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};

// --- ENUMS E ESTRUTURAS DE COMANDO PARA O DISPLAY ---
/**
 * @brief Enumeração dos tipos de comandos que podem ser enviados para a displayTask.
 * Cada tipo de comando corresponde a uma tela ou ação específica no display OLED.
 */
enum DisplayCommandType {
  CMD_CLEAR_DISPLAY,          ///< Limpa a tela do display.
  CMD_SHOW_STATE_INFO,        ///< Exibe o nome do estado atual (geralmente no topo).
  CMD_SHOW_MAIN_MENU_SCREEN,  ///< Exibe a tela do menu principal (IDLE: "1- Iniciar", "2- Sair").
  CMD_SHOW_STARTUP_MESSAGE,   ///< Exibe a mensagem de inicialização ("DisplayTask OK!", "Executando showStartup()").
  CMD_SHOW_RECIPES_LIST,      ///< Exibe a lista de receitas disponíveis (MENU: "1- APA", "2- Witbier", etc.).
  CMD_SHOW_RECIPE_DETAILS_SCREEN, ///< Exibe os detalhes de uma receita específica (etapas, temperaturas, tempos).
  CMD_PRINT_KEYPAD_INPUT      ///< Imprime o texto digitado pelo teclado, geralmente na parte inferior da tela.
};

/**
 * @brief Estrutura de dados para os comandos enviados para a displayTask.
 * Contém o tipo de comando, texto opcional, flag de limpeza de tela e ID da receita.
 */
struct DisplayCommand {
  DisplayCommandType type;      ///< Tipo do comando.
  String text;                  ///< Texto associado ao comando (ex: nome do estado, texto digitado).
  bool clearScreen;             ///< Flag para indicar se a tela deve ser limpa antes de exibir o conteúdo.
  int recipeId;                 ///< ID da receita (para comandos de detalhes de receita).
};

// --- ESTRUTURAS DE DADOS PARA AS RECEITAS ---
/**
 * @brief Estrutura para representar uma única etapa de uma receita de cerveja.
 */
struct RecipeStep {
  String name;        ///< Nome da etapa (ex: "Mostura", "Descanso de Proteína").
  int temperature;    ///< Temperatura da etapa em ºC.
  int duration;       ///< Duração da etapa em minutos.
};

/**
 * @brief Estrutura para representar uma receita completa de cerveja.
 */
struct Recipe {
  String name;        ///< Nome da receita (ex: "American Pale Ale").
  int numSteps;       ///< Número total de etapas nesta receita.
  RecipeStep steps[5];///< Array das etapas da receita (limite de 5 etapas).
};

/**
 * @brief Array constante que armazena todas as receitas pré-configuradas no sistema.
 */
const Recipe recipes[] = {
  // Receita 1: American Pale Ale
  {"American Pale Ale", 2, {
    {"Curva 1", 67, 60},
    {"Curva 2", 76, 10}
  }},
  // Receita 2: Witbier
  {"Witbier", 3, {
    {"Curva 1", 50, 15},
    {"Curva 2", 68, 60},
    {"Curva 3", 76, 10}
  }},
  // Receita 3: Belgian Dubbel
  {"Belgian Dubbel", 4, {
    {"Curva 1", 52, 15},
    {"Curva 2", 64, 45},
    {"Curva 3", 72, 15},
    {"Curva 4", 76, 10}
  }},
  // Receita 4: Bohemian Pilsen
  {"Bohemian Pilsen", 5, {
    {"Curva 1", 45, 15},
    {"Curva 2", 52, 15},
    {"Curva 3", 63, 45},
    {"Curva 4", 72, 15},
    {"Curva 5", 76, 10}
  }},
  // Receita 5: Customizar (apenas um placeholder por enquanto)
  {"Customizar", 0, {}} // Sem etapas definidas ainda
};
/**
 * @brief Número total de receitas pré-configuradas no sistema.
 */
const int NUM_RECIPES = sizeof(recipes) / sizeof(recipes[0]);

// --- FILAS GLOBAIS (Declaradas como 'extern' aqui, definidas em main.cpp) ---
extern QueueHandle_t xDisplayQueue;

/**
 * @brief Implementação da interface de OperationCallback para a máquina de estados Yakindu.
 * Esta classe é responsável por abstrair as operações de hardware e serviços,
 * enviando comandos para as Tasks FreeRTOS apropriadas.
 */
class StatechartCallback : public Statechart::OperationCallback {
public:
  // Propriedades internas da classe para gerenciamento de hardware/estado
  bool oledOK = false; ///< Flag para indicar se o display OLED foi inicializado com sucesso.
  Keypad* keypad = nullptr; ///< Ponteiro para o objeto do teclado matricial.
  bool matrixOK = false; ///< Flag para indicar se o teclado matricial foi inicializado com sucesso.
  String inputBuffer = ""; ///< Buffer para armazenar a entrada digitada pelo teclado.
  unsigned long lastKeyPressTime = 0; ///< Timestamp da última tecla pressionada (para timeout de buffer).

  /**
   * @brief Construtor padrão da classe StatechartCallback.
   */
  StatechartCallback() {}

  /**
   * @brief Define a instância da Statechart para que os callbacks possam disparar eventos.
   * @param sc Ponteiro para o objeto Statechart.
   */
  void setStatechart(Statechart* sc) {
    myStatechart = sc;
  }

  // --- MÉTODOS DE CALLBACK (IMPLEMENTAÇÃO DAS OPERAÇÕES DO YAKINDU) ---

  /**
   * @brief Solicita a inicialização do display OLED na displayTask.
   * Chamado pelo estado INIT_SYSTEM do Yakindu.
   */
  void beginDisplay() override {
    Serial.println("Callback: Solicitando inicio do display (Display Task irá inicializar)");
    // A inicialização real do display é feita na displayTask. Aqui, apenas sinalizamos sucesso.
    oledOK = true; 
  }

  /**
   * @brief Configura o modo de um pino GPIO.
   * Chamado por operações do Yakindu que precisam configurar pinos (ex: LED).
   * @param pin Número do pino.
   * @param mode Modo do pino (OUTPUT ou INPUT).
   */
  void pinMode(sc_integer pin, sc_integer mode) override {
    ::pinMode(pin, mode == 1 ? OUTPUT : INPUT);
  }

  /**
   * @brief Define o nível digital de um pino GPIO.
   * Chamado por operações do Yakindu que controlam saídas digitais (ex: LED).
   * @param pin Número do pino.
   * @param value Valor digital (LOW/HIGH).
   */
  void digitalWrite(sc_integer pin, sc_integer value) override {
    ::digitalWrite(pin, value);
  }

  /**
   * @brief Exibe a mensagem de inicialização do sistema no display.
   * Chamado pelo estado INIT_SYSTEM do Yakindu.
   */
  void showStartup() override {
    DisplayCommand cmd = {CMD_SHOW_STARTUP_MESSAGE};
    xQueueSend(xDisplayQueue, &cmd, portMAX_DELAY);
  }

  /**
   * @brief Exibe o menu de seleção de receitas no display.
   * Chamado pelo estado MENU do Yakindu.
   */
  void showRecipes() override {
    Serial.println("Callback: Executando showRecipes() - Menu de Receitas");
    DisplayCommand cmd = {CMD_SHOW_RECIPES_LIST};
    cmd.clearScreen = true; // Sempre limpa para o menu principal de receitas
    xQueueSend(xDisplayQueue, &cmd, portMAX_DELAY);
    inputBuffer = ""; // Limpa o buffer do teclado para a nova tela
  }

  /**
   * @brief Exibe os detalhes de uma receita específica no display.
   * Chamado pelos estados RECIPE_X do Yakindu.
   * @param recipeId O ID da receita a ser exibida (1-baseado).
   */
  void showRecipe(sc_integer recipeId) override {
    Serial.print("Callback: Executando showRecipe() para receita ID: ");
    Serial.println(recipeId);

    if (recipeId >= 1 && recipeId <= NUM_RECIPES) {
      DisplayCommand cmd = {CMD_SHOW_RECIPE_DETAILS_SCREEN}; // <-- Comando para detalhes da receita
      cmd.recipeId = recipeId - 1; // Ajusta para índice base 0 do array `recipes`
      cmd.clearScreen = true; // Sempre limpa para os detalhes da receita
      xQueueSend(xDisplayQueue, &cmd, portMAX_DELAY);
    } else {
      Serial.println("Callback: Receita inválida solicitada.");
    }
    inputBuffer = ""; // Limpa o buffer do teclado para a nova tela
  }

  // --- MÉTODOS DE CALLBACK PARA FUNÇÕES AINDA NÃO IMPLEMENTADAS OU SIMPLES ---
  void shutdownSystem() override {}
  void initializeProcess() override {}
  void heat(sc_integer) override {}
  void time(sc_integer) override {}
  void showFinished() override {}
  void setTemperature(sc_integer) override {}
  void setTime(sc_integer) override {}
  void initializeSetupProcess() override {}

  /**
   * @brief Exibe o nome do estado atual no Serial Monitor e no display (geralmente no topo).
   * Chamado por várias entradas de estado do Yakindu.
   * @param state String contendo o nome do estado.
   */
  void showState(sc_string state) override {
    Serial.print("Callback: Estado atual: ");
    Serial.println(state);
    // Envia comando para a displayTask para mostrar o nome do estado
    DisplayCommand cmd = {CMD_SHOW_STATE_INFO, String(state)};
    cmd.clearScreen = false; // Não limpa a tela inteira, apenas a linha superior é reescrita
    xQueueSend(xDisplayQueue, &cmd, portMAX_DELAY);
  }

  /**
   * @brief Exibe a tela inicial de IDLE ("Bem-vindo!", "1- Iniciar", "2- Sair").
   * Chamado pelo estado IDLE do Yakindu.
   */
  void showIdleScreen() override {
    Serial.println("Callback: Exibindo tela de IDLE");
    DisplayCommand cmd = {CMD_SHOW_MAIN_MENU_SCREEN};
    cmd.clearScreen = true; // Sempre limpa para esta tela
    xQueueSend(xDisplayQueue, &cmd, portMAX_DELAY);
  }

  /**
   * @brief Inicializa o teclado matricial.
   * Chamado pelo estado IDLE do Yakindu.
   */
  void beginMatrix() override {
    Serial.println("Callback: Iniciando teclado matricial.");
    if (keypad == nullptr) {
      keypad = new Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
    }
    if (keypad != nullptr) {
      matrixOK = true;
      Serial.println("Callback: Teclado iniciado com sucesso.");
    } else {
      Serial.println("Callback: Falha ao iniciar teclado.");
      matrixOK = false;
    }
  }

  // --- FUNÇÕES AUXILIARES PARA GERENCIAMENTO DE ENTRADA DO TECLADO E DISPLAY ---

  /**
   * @brief Lê uma tecla do teclado matricial.
   * Esta função é chamada pela keypadTask.
   * @return O caractere da tecla pressionada, ou NO_KEY se nenhuma tecla.
   */
  char readKeypadChar() {
    if (matrixOK && keypad != nullptr) {
      return keypad->getKey();
    }
    return NO_KEY;
  }

  /**
   * @brief Envia um comando para a displayTask para imprimir o buffer de entrada.
   * @note Esta função é chamada internamente pela stateMachineTask para exibir o input digitado.
   */
  void printKeypadInput() {
    if (inputBuffer.length() > 0) {
      DisplayCommand printCmd = {CMD_PRINT_KEYPAD_INPUT, "Digitado: " + inputBuffer};
      xQueueSend(xDisplayQueue, &printCmd, portMAX_DELAY);
    }
  }

private:
  Statechart* myStatechart = nullptr; ///< Ponteiro para a instância da Statechart.
};

#endif // STATECHARTCALLBACK_H