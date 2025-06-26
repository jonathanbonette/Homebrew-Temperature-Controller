#ifndef STATECHARTCALLBACK_H
#define STATECHARTCALLBACK_H

#include "src-gen/Statechart.h"
#include <Arduino.h>
// DisplayOLED
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// Matrix4x4
#include <Keypad.h>
// --- Sensor de Temperatura DS18B20 ---
#include <OneWire.h>
#include <DallasTemperature.h>
// FreeRTOS Headers
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h" // Para filas

// --- DEFINES DO HARDWARE ---
// Parâmetros do DisplayOLED
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
  CMD_PRINT_KEYPAD_INPUT,      ///< Imprime o texto digitado pelo teclado, geralmente na parte inferior da tela.
  CMD_SHOW_PROCESS_STATUS_SCREEN, ///< Exibe o status atual do processo de cozimento.
  CMD_SHOW_FINISHED_MESSAGE_SCREEN ///< Exibe a mensagem de receita concluída.
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

// --- ENUMS E ESTRUTURAS DE COMANDO PARA O CONTROLE DO PROCESSO ---
/**
 * @brief Enumeração dos tipos de comandos que podem ser enviados para a controlTask.
 * Estes comandos instruem a controlTask sobre qual etapa de receita executar.
 */
enum ControlCommandType {
  CMD_START_RECIPE_STEP, ///< Inicia uma nova etapa da receita com temperatura e duração alvos.
  CMD_ABORT_PROCESS      ///< Aborta o processo de cozimento em andamento.
};

/**
 * @brief Estrutura de dados para os comandos enviados para a controlTask.
 * Contém o tipo de comando e os parâmetros da etapa (temperatura, duração, índice da etapa).
 */
struct ControlCommand {
  ControlCommandType type; ///< Tipo do comando.
  int targetTemperature;   ///< Temperatura alvo para a etapa.
  int durationMinutes;     ///< Duração da etapa em minutos.
  int recipeIndex;         ///< Índice da receita atual.
  int stepIndex;           ///< Índice da etapa atual.
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
 * @brief Estrutura para dados de temperatura do sensor.
 * Útil para enviar leituras de temperatura entre tasks.
 */
struct TemperatureData {
    float temperature1; // Temperatura do sensor 1 (em ºC)
    // float temperature2; // Futuro: Temperatura do sensor 2
    // bool valid1;       // Flag para indicar se a leitura do sensor 1 é válida
    // bool valid2;       // Futuro: Flag para indicar se a leitura do sensor 2 é válida
};

/**
 * @brief Array constante que armazena todas as receitas pré-configuradas no sistema.
 */
const Recipe recipes[] = {
  // Receita 1: American Pale Ale
  {"American Pale Ale", 2, {
    //DEBUG --> mudar os parâmetros no final do projeto
    {"Curva 1", 67, 1}, // {"Curva 1", 67, 60},
    {"Curva 2", 76, 1} // {"Curva 2", 76, 10}
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
extern QueueHandle_t xDisplayQueue; // Fila para exibições no display
extern QueueHandle_t xControlQueue; // Fila para comandos da controlTask
extern QueueHandle_t xSensorQueue; // Fila para receber dados do sensor

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

  // Variáveis para controle da receita e etapa atual em processamento
  // (Inicializadas em -1 para indicar que nenhuma receita está ativa)
  int currentRecipeIdx = -1; 
  int currentStepIdx = -1;

  // --- OBJETOS PARA O DS18B20 ---
  OneWire* oneWireBus = nullptr;
  DallasTemperature* waterThermometer = nullptr;
  
  float lastReadTemperature = 0.0; // Armazena a última temperatura lida para ser acessada


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
    // A inicialização real do display é feita na displayTask. Aqui é apenas um debug para sinalizar sucesso.
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
      DisplayCommand cmd = {CMD_SHOW_RECIPE_DETAILS_SCREEN}; // Comando para detalhes da receita
      cmd.recipeId = recipeId - 1; // Ajusta para índice base 0 do array `recipes`
      cmd.clearScreen = true; // Sempre limpa para os detalhes da receita
      xQueueSend(xDisplayQueue, &cmd, portMAX_DELAY);
    } else {
      Serial.println("Callback: Receita inválida solicitada.");
    }
    inputBuffer = ""; // Limpa o buffer do teclado para a nova tela
  }

  // --- MÉTODOS DE CALLBACK PARA O STANDARD_PROCESS ---

  /**
   * @brief Inicializa o processo de cozimento.
   * Chamado pelo estado START_PROCESS do Yakindu.
   */
  void initializeProcess() override {
    Serial.println("Callback: Processo de cozimento inicializado.");
    // TODO: Aqui faria quaisquer inicializações de atuadores/sensores antes do ciclo de controle.
    // Ex: garantir que resistências e mixer estejam desligados inicialmente.
    // digital_write(heater_pin, LOW);
    // digital_write(mixer_pin, LOW);
  }

  /**
   * @brief Inicia ou avança para a próxima etapa da receita sendo processada.
   * Esta operação é chamada pelo Yakindu na entrada de CONTROL_PROCESS_LOOP.
   * Ela é responsável por configurar o hardware para a etapa atual e pode
   * sinalizar o início da monitoração de temperatura/tempo.
   * @param recipeIndex O índice da receita que está sendo processada (0-baseado).
   */
  void startNextRecipeStep(sc_integer recipeIndex) override {
    // Se a receita mudou ou é o início do processo (vinda do START_PROCESS)
    if (this->currentRecipeIdx != recipeIndex) {
        this->currentRecipeIdx = recipeIndex;
        this->currentStepIdx = 0; // Começa da primeira etapa
    } else {
        // Se já está na mesma receita, avança para a próxima etapa
        this->currentStepIdx++; 
    }

    if (this->currentRecipeIdx >= 0 && this->currentRecipeIdx < NUM_RECIPES) {
        const Recipe& recipe = recipes[this->currentRecipeIdx];
        if (this->currentStepIdx < recipe.numSteps) {
            const RecipeStep& step = recipe.steps[this->currentStepIdx];
            Serial.printf("Callback: INICIANDO ETAPA %d/%d: %s (Temp: %dC, Tempo: %dmin)\n",
                          this->currentStepIdx + 1, recipe.numSteps, step.name.c_str(), step.temperature, step.duration);

            // TODO: Aqui é onde enviaria comandos para as tasks de controle de hardware.
            // Por enquanto, apenas logs e exibição de status.
            // Por exemplo, enviar um comando para a futura 'controlTask':
            // CommandToControlTask cmd = {CMD_SET_TARGET_TEMP, step.temperature, step.duration};
            // xQueueSend(xControlQueue, &cmd, portMAX_DELAY);
            // AGORA, ENVIA O COMANDO PARA A CONTROL TASK AQUI!
            ControlCommand controlCmd = {CMD_START_RECIPE_STEP};
            controlCmd.recipeIndex = this->currentRecipeIdx;
            controlCmd.stepIndex = this->currentStepIdx;
            controlCmd.targetTemperature = step.temperature; // Passa a temperatura alvo
            controlCmd.durationMinutes = step.duration;     // Passa a duração
            xQueueSend(xControlQueue, &controlCmd, portMAX_DELAY); // Envia o comando para a ControlTask


            // Exibe o status inicial da etapa no display
            // Os valores de temperatura atual e tempo restante serão atualizados por uma tarefa de controle.
                showProcessStatus(0, step.temperature, step.duration, 0, const_cast<sc_string>(step.name.c_str()), this->currentStepIdx + 1, recipe.numSteps);


        } else {
            // Este bloco não deveria ser alcançado se as guardas do Yakindu estiverem corretas.
            // Significa que startNextRecipeStep foi chamado mas não há mais etapas.
            Serial.println("Callback: Erro lógico: startNextRecipeStep chamada sem mais etapas.");
        }
    } else {
        Serial.println("Callback: Erro: Receita inválida em startNextRecipeStep.");
        // TODO: Lógica de erro ou transição de falha na máquina de estados.
    }
  }

  /**
   * @brief Verifica se ainda há etapas a serem processadas na receita atual.
   * Implementa a operação hasMoreSteps() do Yakindu (usada nas guardas de transição).
   * @return true se houver mais etapas, false caso contrário.
   */
  sc_boolean hasMoreSteps() override {
    if (currentRecipeIdx >= 0 && currentRecipeIdx < NUM_RECIPES) {
      return (currentStepIdx + 1) < recipes[currentRecipeIdx].numSteps;
    }
    return false; // Se não houver receita selecionada ou etapas inválidas, não há mais etapas.
  }

   /**
   * @brief Retorna o índice da receita atualmente em processamento (0-baseado).
   * Implementa a operação getCurrentRecipeIndex() do Yakindu.
   * @return O índice da receita.
   */
  sc_integer getCurrentRecipeIndex() override {
    return currentRecipeIdx;
  }

  /**
   * @brief Retorna o índice da etapa atual dentro da receita (0-baseado).
   * Implementa a operação getCurrentStepIndex() do Yakindu.
   * @return O índice da etapa.
   */
  sc_integer getCurrentStepIndex() override {
    return currentStepIdx;
  }

  /**
   * @brief Exibe o status do processo de cozimento no display OLED.
   * Chamado pela máquina de estados ou por tarefas de controle para atualizar o UI.
   * @param currentTemp Temperatura atual lida do sensor.
   * @param targetTemp Temperatura alvo para a etapa atual.
   * @param remainingTime Tempo restante para a etapa atual (em segundos ou minutos).
   * @param stepName Nome da etapa atual (ex: "Mostura").
   * @param stepNum Número da etapa atual (1-baseado, ex: 1 de 2).
   * @param totalSteps Número total de etapas da receita.
   */
  void showProcessStatus(sc_integer currentTemp, sc_integer targetTemp, sc_integer remainingMinutes, sc_integer remainingSeconds, sc_string stepName, sc_integer stepNum, sc_integer totalSteps) override {
      Serial.printf("Callback: Status Processo (Display): Etapa %d/%d '%s' - Atual: %dC, Alvo: %dC, Restante: %dmin %02ds\n",
                    stepNum, totalSteps, stepName, currentTemp, targetTemp, remainingMinutes, remainingSeconds);

      DisplayCommand cmd = {CMD_SHOW_PROCESS_STATUS_SCREEN};
      cmd.clearScreen = true; // Geralmente, uma tela de status é limpa para cada atualização

      // Declara e formata o timeBuffer
      char timeBuffer[10]; // Buffer para formatar o tempo (ex: "1 m 00 s")
      sprintf(timeBuffer, "%d m %02d s", remainingMinutes, remainingSeconds); // Formata segundos com 2 dígitos

      
      // Formata a string de status com as informações.
      // Use várias linhas para melhor legibilidade no OLED.
      cmd.text = String("Receita: ") + recipes[currentRecipeIdx].name + "\n" +
                 "Etapa " + stepNum + "/" + totalSteps + ": " + String(stepName) + "\n" +
                 "Temp: " + currentTemp + "C / " + targetTemp + "C\n" +
                 "Tempo: " + String(timeBuffer);

      xQueueSend(xDisplayQueue, &cmd, portMAX_DELAY);
  }

  /**
 * @brief Sinaliza o término do processo e desliga atuadores.
 * Chamado pelo estado FINISH_PROCESS do Yakindu.
 */
  void showFinished() override {
      Serial.println("Callback: Processo de cozimento finalizado. Desligando atuadores.");
      // TODO: Implementar lógica para desligar resistências e mixer aqui.
      // myStatechart->digitalWrite(heater_pin, LOW);
      // myStatechart->digitalWrite(mixer_pin, LOW);

      // Após desligar os atuadores (simulado por enquanto), disparar o evento finished_process
      // para a máquina de estados transitar para FINISHED_MESSAGE.
      if (myStatechart != nullptr) { // Garante que o ponteiro está válido
          myStatechart->raiseFinished_process(); // <--- DISPARA O EVENTO AQUI!
      } else {
          Serial.println("Callback: Erro: myStatechart é nullptr em showFinished!");
      }

      // Resetar índices de receita/etapa após o fim do processo
      currentRecipeIdx = -1;
      currentStepIdx = -1;
  }

   /**
   * @brief Exibe a mensagem de receita concluída no display.
   * Chamado pelo estado FINISHED_MESSAGE do Yakindu.
   */
  void showFinishedMessage() override {
      Serial.println("Callback: Exibindo mensagem 'Receita concluída'.");
      DisplayCommand cmd = {CMD_SHOW_FINISHED_MESSAGE_SCREEN};
      cmd.clearScreen = true; // Limpa a tela para exibir a mensagem final
      xQueueSend(xDisplayQueue, &cmd, portMAX_DELAY);
  }

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

  // --- SEMÁFORO ---
  /**
   * @brief Inicializa os pinos do módulo semáforo LED.
   * Chamado pelo estado IDLE do Yakindu.
   */
  void beginSemaphore() override {
    Serial.println("Callback: Inicializando pinos do semáforo.");

    if (myStatechart != nullptr) {
        // Obter os valores dos pinos e modo de saída definidos no Yakindu
        int redPin = myStatechart->getSemaphore_red_pin();
        int yellowPin = myStatechart->getSemaphore_yellow_pin();
        int greenPin = myStatechart->getSemaphore_green_pin();
        int outputMode = myStatechart->getOutput(); // Obter o valor de 'output' (1)
        int lowValue = myStatechart->getLow();     // Obter o valor de 'low' (0)

        // CHAMAR AS IMPLEMENTAÇÕES DOS MÉTODOS DE CALLBACK DIRETAMENTE
        // vai ser usado as implementações de ::pinMode e ::digitalWrite
        pinMode(redPin, outputMode);
        pinMode(yellowPin, outputMode);
        pinMode(greenPin, outputMode);
        
        // E desligue todos os LEDs inicialmente para garantir um estado conhecido
        digitalWrite(redPin, lowValue);
        digitalWrite(yellowPin, lowValue);
        digitalWrite(greenPin, lowValue);

        Serial.printf("Callback: Semaforo R:%d Y:%d G:%d configurados como OUTPUT e desligados.\n", redPin, yellowPin, greenPin);
    } else {
        Serial.println("Callback: ERRO: myStatechart e nullptr ao inicializar semaforo.");
    }
  }

  // --- MÉTODO DE CALLBACK PARA O SENSOR DE ÁGUA ---
  /**
   * @brief Inicializa o sensor de temperatura DS18B20.
   * Chamado pelo estado INIT_SYSTEM do Yakindu.
   */
  void beginWaterSensor() override {
    Serial.println("Callback: Inicializando sensor de temperatura DS18B20.");
    if (myStatechart != nullptr) {
        int waterSensorPin = myStatechart->getWater_sensor_pin(); // Pego do Yakindu
        Serial.printf("Callback: Sensor DS18B20 no GPIO%d.\n", waterSensorPin);

        // Cria a instância OneWire na heap (já que não pode usar o construtor aqui diretamente)
        // e a instância DallasTemperature
        if (oneWireBus == nullptr) {
          oneWireBus = new OneWire(waterSensorPin);
        }
        if (waterThermometer == nullptr) {
          waterThermometer = new DallasTemperature(oneWireBus);
        }

        if (waterThermometer != nullptr) {
          waterThermometer->begin(); // Inicia a comunicação com o sensor
          Serial.println("Callback: Sensor DS18B20 inicializado.");
        } else {
          Serial.println("Callback: ERRO! Falha ao criar objeto DallasTemperature.");
        }
    } else {
        Serial.println("Callback: ERRO: myStatechart e nullptr ao inicializar sensor de agua.");
    }
  }

  /**
   * @brief Lê a temperatura atual do sensor DS18B20.
   * Esta função é chamada pela temperatureSensorTask.
   * @return A temperatura lida em float. Retorna -1000.0 se houver erro.
   */
  float readWaterTemperature() {
      if (waterThermometer != nullptr) {
          waterThermometer->requestTemperatures(); // Solicita a leitura da temperatura
          float tempC = waterThermometer->getTempCByIndex(0); // Lê a temperatura do primeiro sensor encontrado

          if (tempC != DEVICE_DISCONNECTED_C) { // Verifica se a leitura foi bem-sucedida
              lastReadTemperature = tempC;
              return tempC;
          } else {
              Serial.println("Callback: ERRO! Sensor DS18B20 desconectado ou leitura falhou.");
              return -1000.0; // Valor de erro
          }
      }
      return -1000.0; // Objeto do sensor não inicializado
  }

  // --- MÉTODOS DE CALLBACK PARA FUNÇÕES AINDA NÃO IMPLEMENTADAS OU SIMPLES ---
  void shutdownSystem() override {}
  void heat(sc_integer) override {} // TODO: Será implementado para controlar o aquecedor
  void time(sc_integer) override {} // TODO: Será implementado para controlar o tempo de etapa
  void setTemperature(sc_integer) override {}
  void setTime(sc_integer) override {}
  void initializeSetupProcess() override {}

private:
  Statechart* myStatechart = nullptr; ///< Ponteiro para a instância da Statechart.
};

#endif // STATECHARTCALLBACK_H