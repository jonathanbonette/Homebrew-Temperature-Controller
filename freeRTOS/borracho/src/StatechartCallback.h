/**
 * @file StatechartCallback.h
 * @brief Definição da interface de callbacks para a máquina de estados Itemis.
 * @details Esta classe implementa as operações de callback definidas no modelo Itemis.
 * Ela serve como uma camada de abstração entre a lógica da máquina de estados e as
 * operações de baixo nível do hardware, como controle de display, leitura de teclado,
 * controle de PWM e comunicação com outras tarefas FreeRTOS.
 * @author Jonathan Chrysostomo Cabral Bonette
 * @date 26/07/2025
 * @copyright Copyright (c) 2025
 */

#ifndef STATECHARTCALLBACK_H
#define STATECHARTCALLBACK_H

// Includes do projeto
#include "src-gen/Statechart.h"
#include <Arduino.h>

// DisplayOLED
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Matrix4x4
#include <Keypad.h>

// FreeRTOS Headers
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// --- DEFINES DO HARDWARE ---
// Parâmetros do DisplayOLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// --- DEFINES ESPECÍFICAS DO PWM LEDC (ESP32) ---
#define LEDC_CHANNEL_PWM_HEATER 0

// Mapeamento do teclado 4x4
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};

// --- ENUMS E ESTRUTURAS DE COMANDO ---
/**
 * @brief Enumeração dos tipos de comandos para o display.
 */
enum DisplayCommandType
{
  CMD_CLEAR_DISPLAY,               ///< Limpa a tela do display
  CMD_SHOW_STATE_INFO,             ///< Exibe o nome do estado atual (geralmente no topo)
  CMD_SHOW_MAIN_MENU_SCREEN,       ///< Exibe a tela do menu principal (IDLE: "1- Iniciar", "2- Sair")
  CMD_SHOW_STARTUP_MESSAGE,        ///< Exibe a mensagem de inicialização ("DisplayTask OK!", "Executando showStartup()")
  CMD_SHOW_RECIPES_LIST,           ///< Exibe a lista de receitas disponíveis (MENU: "1- APA", "2- Witbier", etc.)
  CMD_SHOW_RECIPE_DETAILS_SCREEN,  ///< Exibe os detalhes de uma receita específica (etapas, temperaturas, tempos)
  CMD_PRINT_KEYPAD_INPUT,          ///< Imprime o texto digitado pelo teclado, geralmente na parte inferior da tela
  CMD_SHOW_PROCESS_STATUS_SCREEN,  ///< Exibe o status atual do processo de cozimento
  CMD_SHOW_FINISHED_MESSAGE_SCREEN ///< Exibe a mensagem de receita concluída
};

/**
 * @brief Estrutura para os comandos de display, enviados via fila.
 */
struct DisplayCommand
{
  DisplayCommandType type; // Tipo do comando
  String text;             // Texto associado ao comando (ex: nome do estado, texto digitado)
  bool clearScreen;        // Flag para indicar se a tela deve ser limpa antes de exibir o conteúdo
  int recipeId;            // ID da receita (para comandos de detalhes de receita)
};

/**
 * @brief Enumeração dos tipos de comandos para o controle do processo.
 */
enum ControlCommandType
{
  CMD_START_RECIPE_STEP, // Inicia uma nova etapa da receita com temperatura e duração alvos
  CMD_ABORT_PROCESS      // Aborta o processo de cozimento em andamento
};

/**
 * @brief Estrutura para os comandos de controle do processo.
 */
struct ControlCommand
{
  ControlCommandType type; // Tipo do comando
  int targetTemperature;   // Temperatura alvo para a etapa
  int durationMinutes;     // Duração da etapa em minutos
  int recipeIndex;         // Índice da receita atual
  int stepIndex;           // Índice da etapa atual
};

// --- ESTRUTURAS DE DADOS PARA AS RECEITAS ---
/**
 * @brief Estrutura para uma única etapa de uma receita.
 */
struct RecipeStep
{
  String name;     // Nome da etapa (ex: "Mostura", "Descanso de Proteína")
  int temperature; // Temperatura da etapa em ºC
  int duration;    // Duração da etapa em minutos
};

/**
 * @brief Estrutura para uma receita completa.
 */
struct Recipe
{
  String name;         // Nome da receita (ex: "American Pale Ale")
  int numSteps;        // Número total de etapas nesta receita
  RecipeStep steps[5]; // Array das etapas da receita (limite de 5 etapas)
};

/**
 * @brief Estrutura para dados de temperatura.
 */
struct TemperatureData
{
  float temperature1; // Temperatura do sensor 1 (em ºC)
};

/**
 * @brief Array de receitas pré-configuradas.
 */
const Recipe recipes[] = {
    // Receita 1: American Pale Ale
    {"American Pale Ale", 2, { {"Curva 1", 67, 1}, {"Curva 2", 76, 1}}}, // {"Curva 1", 67, 60}, {"Curva 2", 76, 10}
    // Receita 2: Witbier
    {"Witbier", 3, {{"Curva 1", 50, 15}, {"Curva 2", 68, 60}, {"Curva 3", 76, 10}}},
    // Receita 3: Belgian Dubbel
    {"Belgian Dubbel", 4, {{"Curva 1", 52, 15}, {"Curva 2", 64, 45}, {"Curva 3", 72, 15}, {"Curva 4", 76, 10}}},
    // Receita 4: Bohemian Pilsen
    {"Bohemian Pilsen", 5, {{"Curva 1", 45, 15}, {"Curva 2", 52, 15}, {"Curva 3", 63, 45}, {"Curva 4", 72, 15}, {"Curva 5", 76, 10}}},
    // Receita 5: Customizar (apenas um placeholder por enquanto)
    {"Customizar", 0, {}} // Sem etapas definidas ainda
};

const int NUM_RECIPES = sizeof(recipes) / sizeof(recipes[0]);

// --- FILAS GLOBAIS ---
extern QueueHandle_t xDisplayQueue; // Fila para exibições no display
extern QueueHandle_t xControlQueue; // Fila para comandos da controlTask
extern QueueHandle_t xSensorQueue;  // Fila para receber dados do sensor

/**
 * @brief Implementação da interface de OperationCallback para a máquina de estados.
 * @details Esta classe gerencia a comunicação entre a máquina de estados e o hardware,
 * utilizando filas FreeRTOS para interagir com outras tarefas de forma segura.
 */
class StatechartCallback : public Statechart::OperationCallback
{
public:
  // Propriedades internas para estado de hardware e UI
  bool oledOK = false;                // Flag para indicar se o display OLED foi inicializado com sucesso
  Keypad *keypad = nullptr;           // Ponteiro para o objeto do teclado matricial
  bool matrixOK = false;              // Flag para indicar se o teclado matricial foi inicializado com sucesso
  String inputBuffer = "";            // Buffer para armazenar a entrada digitada pelo teclado
  unsigned long lastKeyPressTime = 0; // Timestamp da última tecla pressionada (para timeout de buffer)

  // Variáveis para controle da receita e etapa atual em processamento
  // (Inicializadas em -1 para indicar que nenhuma receita está ativa)
  int currentRecipeIdx = -1;
  int currentStepIdx = -1;

  float lastReadTemperature = 0.0; // Armazena a última temperatura lida para ser acessada

  /**
   * @brief Construtor padrão da classe.
   */
  StatechartCallback() {}

  /**
   * @brief Define a instância da Statechart.
   * @param sc Ponteiro para o objeto Statechart.
   */
  void setStatechart(Statechart *sc)
  {
    myStatechart = sc;
  }

  // --- MÉTODOS DE CALLBACK (IMPLEMENTAÇÃO DAS OPERAÇÕES DO ITEMIS) ---
  /**
   * @brief Solicita a inicialização do display OLED na displayTask.
   * Chamado pelo estado INIT_SYSTEM do Itemis.
   */
  void beginDisplay() override
  {
    Serial.println("Callback: Solicitando inicio do display (Display Task irá inicializar)");
    // A inicialização real do display é feita na displayTask. Aqui é apenas um debug para sinalizar sucesso
    oledOK = true;
  }

  /**
   * @brief Configura o modo de um pino GPIO.
   * Chamado por operações do Itemis que precisam configurar pinos (ex: LED).
   * @param pin Número do pino.
   * @param mode Modo do pino (OUTPUT ou INPUT).
   */
  void pinMode(sc_integer pin, sc_integer mode) override
  {
    ::pinMode(pin, mode == 1 ? OUTPUT : INPUT);
  }

  /**
   * @brief Define o nível digital de um pino GPIO.
   * Chamado por operações do Itemis que controlam saídas digitais (ex: LED).
   * @param pin Número do pino.
   * @param value Valor digital (LOW/HIGH).
   */
  void digitalWrite(sc_integer pin, sc_integer value) override
  {
    ::digitalWrite(pin, value);
  }

  /**
   * @brief Exibe a mensagem de inicialização do sistema no display.
   * Chamado pelo estado INIT_SYSTEM do Itemis.
   */
  void showStartup() override
  {
    DisplayCommand cmd = {CMD_SHOW_STARTUP_MESSAGE};
    xQueueSend(xDisplayQueue, &cmd, portMAX_DELAY);
  }

  /**
   * @brief Exibe o menu de seleção de receitas no display.
   * Chamado pelo estado MENU do Itemis.
   */
  void showRecipes() override
  {
    Serial.println("Callback: Executando showRecipes() - Menu de Receitas");
    DisplayCommand cmd = {CMD_SHOW_RECIPES_LIST};
    cmd.clearScreen = true; // Sempre limpa para o menu principal de receitas
    xQueueSend(xDisplayQueue, &cmd, portMAX_DELAY);
    inputBuffer = ""; // Limpa o buffer do teclado para a nova tela
  }

  /**
   * @brief Exibe os detalhes de uma receita específica no display.
   * Chamado pelos estados RECIPE_X do Itemis.
   * @param recipeId O ID da receita a ser exibida (1-baseado).
   */
  void showRecipe(sc_integer recipeId) override
  {
    Serial.print("Callback: Executando showRecipe() para receita ID: ");
    Serial.println(recipeId);

    if (recipeId >= 1 && recipeId <= NUM_RECIPES)
    {
      DisplayCommand cmd = {CMD_SHOW_RECIPE_DETAILS_SCREEN}; // Comando para detalhes da receita
      cmd.recipeId = recipeId - 1;                           // Ajusta para índice base 0 do array `recipes`
      cmd.clearScreen = true;                                // Sempre limpa para os detalhes da receita
      xQueueSend(xDisplayQueue, &cmd, portMAX_DELAY);
    }
    else
    {
      Serial.println("Callback: Receita inválida solicitada.");
    }
    inputBuffer = ""; // Limpa o buffer do teclado para a nova tela
  }

  // --- MÉTODOS DE CALLBACK PARA O STANDARD_PROCESS ---
  /**
   * @brief Inicializa o processo de cozimento.
   * Chamado pelo estado START_PROCESS do Itemis.
   */
  void initializeProcess() override
  {
    Serial.println("Callback: Processo de cozimento inicializado.");
  }

  /**
   * @brief Inicia ou avança para a próxima etapa da receita sendo processada.
   * Esta operação é chamada pelo Itemis na entrada de CONTROL_PROCESS_LOOP.
   * Ela é responsável por configurar o hardware para a etapa atual e pode
   * sinalizar o início da monitoração de temperatura/tempo.
   * @param recipeIndex O índice da receita que está sendo processada (0-baseado).
   */
  void startNextRecipeStep(sc_integer recipeIndex) override
  {
    // Se a receita mudou ou é o início do processo (vinda do START_PROCESS)
    if (this->currentRecipeIdx != recipeIndex)
    {
      this->currentRecipeIdx = recipeIndex;
      this->currentStepIdx = 0; // Começa da primeira etapa
    }
    else
    {
      // Se já está na mesma receita, avança para a próxima etapa
      this->currentStepIdx++;
    }

    if (this->currentRecipeIdx >= 0 && this->currentRecipeIdx < NUM_RECIPES)
    {
      const Recipe &recipe = recipes[this->currentRecipeIdx];
      if (this->currentStepIdx < recipe.numSteps)
      {
        const RecipeStep &step = recipe.steps[this->currentStepIdx];
        Serial.printf("Callback: INICIANDO ETAPA %d/%d: %s (Temp: %dC, Tempo: %dmin)\n",
                      this->currentStepIdx + 1, recipe.numSteps, step.name.c_str(), step.temperature, step.duration);

        ControlCommand controlCmd = {CMD_START_RECIPE_STEP};
        controlCmd.recipeIndex = this->currentRecipeIdx;
        controlCmd.stepIndex = this->currentStepIdx;
        controlCmd.targetTemperature = step.temperature;       // Passa a temperatura alvo
        controlCmd.durationMinutes = step.duration;            // Passa a duração
        xQueueSend(xControlQueue, &controlCmd, portMAX_DELAY); // Envia o comando para a ControlTask

        // Exibe o status inicial da etapa no display
        // Os valores de temperatura atual e tempo restante serão atualizados por uma tarefa de controle
        showProcessStatus(0, step.temperature, step.duration, 0, const_cast<sc_string>(step.name.c_str()), this->currentStepIdx + 1, recipe.numSteps, true);
      }
      else
      {
        // Este bloco não deveria ser alcançado se as guardas do Itemis estiverem corretas
        // Significa que startNextRecipeStep foi chamado mas não há mais etapas
        Serial.println("Callback: Erro lógico: startNextRecipeStep chamada sem mais etapas.");
      }
    }
    else
    {
      Serial.println("Callback: Erro: Receita inválida em startNextRecipeStep.");
    }
  }

  /**
   * @brief Verifica se ainda há etapas a serem processadas na receita atual.
   * Implementa a operação hasMoreSteps() do Itemis (usada nas guardas de transição).
   * @return true se houver mais etapas, false caso contrário.
   */
  sc_boolean hasMoreSteps() override
  {
    if (currentRecipeIdx >= 0 && currentRecipeIdx < NUM_RECIPES)
    {
      return (currentStepIdx + 1) < recipes[currentRecipeIdx].numSteps;
    }
    return false; // Se não houver receita selecionada ou etapas inválidas, não há mais etapas
  }

  /**
   * @brief Retorna o índice da receita atualmente em processamento (0-baseado).
   * Implementa a operação getCurrentRecipeIndex() do Itemis.
   * @return O índice da receita.
   */
  sc_integer getCurrentRecipeIndex() override
  {
    return currentRecipeIdx;
  }

  /**
   * @brief Retorna o índice da etapa atual dentro da receita (0-baseado).
   * Implementa a operação getCurrentStepIndex() do Itemis.
   * @return O índice da etapa.
   */
  sc_integer getCurrentStepIndex() override
  {
    return currentStepIdx;
  }

  /**
   * @brief Exibe o status do processo de cozimento no display OLED.
   * Chamado pela máquina de estados ou por tarefas de controle para atualizar o UI.
   * @param currentTemp Temperatura atual lida do sensor.
   * @param targetTemp Temperatura alvo para a etapa atual.
   * @param remainingMinutes Tempo restante para a etapa atual (em minutos).
   * @param remainingSeconds Tempo restante para a etapa atual (em segundos).
   * @param stepName Nome da etapa atual (ex: "Mostura").
   * @param stepNum Número da etapa atual (1-baseado, ex: 1 de 2).
   * @param totalSteps Número total de etapas da receita.
   * @param isRamping Indica se a etapa está em fase de rampa (esperando atingir o setpoint). // <--- NOVO PARÂMETRO
   */
  void showProcessStatus(sc_integer currentTemp, sc_integer targetTemp, sc_integer remainingMinutes, sc_integer remainingSeconds, sc_string stepName, sc_integer stepNum, sc_integer totalSteps, sc_boolean isRamping) override
  {
    // Debug
    Serial.printf("Callback: Status Processo: Etapa %d/%d '%s' - Atual: %dC, Alvo: %dC. Tempo: %d:%02d (Rampa: %s)\n",
                  stepNum, totalSteps, stepName, currentTemp, targetTemp, remainingMinutes, remainingSeconds, isRamping ? "SIM" : "NAO");

    DisplayCommand cmd = {CMD_SHOW_PROCESS_STATUS_SCREEN};
    cmd.clearScreen = true; // Sempre limpa para esta tela de status

    // --- LÓGICA DE MONTAGEM DA STRING AGORA DENTRO DESTE CALLBACK ---
    String statusMessage_part1 = "Receita: " + recipes[currentRecipeIdx].name;
    String statusMessage_part2 = "Etapa " + String(stepNum) + "/" + String(totalSteps) + ": " + String(stepName);
    String statusMessage_part3; // Linha da temperatura
    String statusMessage_part4; // Linha do tempo ou rampa

    if (isRamping)
    { // Se está em fase de rampa
      statusMessage_part3 = "Rampa: " + String(currentTemp) + "C / " + String(targetTemp) + "C";
      statusMessage_part4 = "Aguardando Setpoint...";
    }
    else
    { // Se atingiu o setpoint, mostra temp e tempo
      statusMessage_part3 = "Temp: " + String(currentTemp) + "C / " + String(targetTemp) + "C";
      char timeBuffer[10];
      sprintf(timeBuffer, "%d m %02d s", remainingMinutes, remainingSeconds);
      statusMessage_part4 = "Tempo: " + String(timeBuffer);
    }

    cmd.text = statusMessage_part1 + "\n" + statusMessage_part2 + "\n" + statusMessage_part3 + "\n" + statusMessage_part4;
    xQueueSend(xDisplayQueue, &cmd, portMAX_DELAY); // Envia para a displayTask
  }

  /**
   * @brief Sinaliza o término do processo e desliga atuadores.
   * Chamado pelo estado FINISH_PROCESS do Itemis.
   */
  void showFinished() override
  {
    Serial.println("Callback: Processo de cozimento finalizado. Desligando atuadores.");
    // Após desligar os atuadores, disparar o evento finished_process
    // para a máquina de estados transitar para FINISHED_MESSAGE
    if (myStatechart != nullptr)
    {
      myStatechart->raiseFinished_process();
    }
    else
    {
      Serial.println("Callback: Erro: myStatechart é nullptr em showFinished!");
    }

    // Resetar índices de receita/etapa após o fim do processo
    currentRecipeIdx = -1;
    currentStepIdx = -1;
  }

  /**
   * @brief Exibe a mensagem de receita concluída no display.
   * Chamado pelo estado FINISHED_MESSAGE do Itemis.
   */
  void showFinishedMessage() override
  {
    Serial.println("Callback: Exibindo mensagem 'Receita concluída'.");
    DisplayCommand cmd = {CMD_SHOW_FINISHED_MESSAGE_SCREEN};
    cmd.clearScreen = true; // Limpa a tela para exibir a mensagem final
    xQueueSend(xDisplayQueue, &cmd, portMAX_DELAY);
  }

  /**
   * @brief Exibe o nome do estado atual no Serial Monitor e no display (geralmente no topo).
   * Chamado por várias entradas de estado do Itemis.
   * @param state String contendo o nome do estado.
   */
  void showState(sc_string state) override
  {
    Serial.print("Callback: Estado atual: ");
    Serial.println(state);
    // Envia comando para a displayTask para mostrar o nome do estado
    DisplayCommand cmd = {CMD_SHOW_STATE_INFO, String(state)};
    cmd.clearScreen = false; // Não limpa a tela inteira, apenas a linha superior é reescrita
    xQueueSend(xDisplayQueue, &cmd, portMAX_DELAY);
  }

  /**
   * @brief Exibe a tela inicial de IDLE ("Bem-vindo!", "1- Iniciar", "2- Sair").
   * Chamado pelo estado IDLE do Itemis.
   */
  void showIdleScreen() override
  {
    Serial.println("Callback: Exibindo tela de IDLE");
    DisplayCommand cmd = {CMD_SHOW_MAIN_MENU_SCREEN};
    cmd.clearScreen = true; // Sempre limpa para esta tela
    xQueueSend(xDisplayQueue, &cmd, portMAX_DELAY);
  }

  /**
   * @brief Inicializa o teclado matricial.
   * Chamado pelo estado IDLE do Itemis.
   */
  void beginMatrix() override
  {
    Serial.println("Callback: Iniciando teclado matricial.");
    if (keypad == nullptr)
    {
      keypad = new Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
    }
    if (keypad != nullptr)
    {
      matrixOK = true;
      Serial.println("Callback: Teclado iniciado com sucesso.");
    }
    else
    {
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
  char readKeypadChar()
  {
    if (matrixOK && keypad != nullptr)
    {
      return keypad->getKey();
    }
    return NO_KEY;
  }

  /**
   * @brief Envia um comando para a displayTask para imprimir o buffer de entrada.
   * @note Esta função é chamada internamente pela stateMachineTask para exibir o input digitado.
   */
  void printKeypadInput()
  {
    if (inputBuffer.length() > 0)
    {
      DisplayCommand printCmd = {CMD_PRINT_KEYPAD_INPUT, "Digitado: " + inputBuffer};
      xQueueSend(xDisplayQueue, &printCmd, portMAX_DELAY);
    }
  }

  // --- SEMÁFORO ---
  /**
   * @brief Inicializa os pinos do módulo semáforo LED.
   * Chamado pelo estado IDLE do Itemis.
   */
  void beginSemaphore() override
  {
    Serial.println("Callback: Inicializando pinos do semáforo.");

    if (myStatechart != nullptr)
    {
      // Obter os valores dos pinos e modo de saída definidos no Itemis
      int redPin = myStatechart->getSemaphore_red_pin();
      int yellowPin = myStatechart->getSemaphore_yellow_pin();
      int greenPin = myStatechart->getSemaphore_green_pin();
      int outputMode = myStatechart->getOutput(); // Obter o valor de 'output' (1)
      int lowValue = myStatechart->getLow();      // Obter o valor de 'low' (0)

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
    }
    else
    {
      Serial.println("Callback: ERRO: myStatechart e nullptr ao inicializar semaforo.");
    }
  }

  // --- MÉTODO DE CALLBACK PARA CONTROLE DO AQUECEDOR PWM ---
  /**
   * @brief Configura o pino GPIO especificado para operar como saída PWM para o aquecedor.
   * Chamado pelo Itemis (ex: no INIT_SYSTEM).
   */
  void setupHeaterPWM() override
  {
    Serial.println("Callback: Configurando PWM do aquecedor.");
    if (myStatechart != nullptr)
    {
      int heaterPin = myStatechart->getHeater_pwm_pin();       // Pego do Itemis
      int pwmFreq = myStatechart->getPwm_frequency();          // Pego do Itemis
      int pwmResBits = myStatechart->getPwm_resolution_bits(); // Pego do Itemis

      // Configura o timer LEDC do ESP32
      ledcSetup(LEDC_CHANNEL_PWM_HEATER, pwmFreq, pwmResBits);
      // Anexa o pino ao canal PWM
      ledcAttachPin(heaterPin, LEDC_CHANNEL_PWM_HEATER);
      // Garante que o PWM começa desligado (0% duty cycle)
      ledcWrite(LEDC_CHANNEL_PWM_HEATER, 0);

      Serial.printf("Callback: PWM do aquecedor configurado no GPIO%d (Freq: %dHz, Res: %d bits). Canal %d.\n",
                    heaterPin, pwmFreq, pwmResBits, LEDC_CHANNEL_PWM_HEATER);
    }
    else
    {
      Serial.println("Callback: ERRO: myStatechart é nullptr ao configurar PWM do aquecedor.");
    }
  }

  /**
   * @brief Controla o aquecedor através do sinal PWM.
   * Esta função é chamada pela operação 'heat' do Itemis e pela 'controlTask'.
   * @param duty_cycle O ciclo de trabalho do PWM (valor de 0 até (2^resolution)-1).
   */
  void controlHeaterPWM(sc_integer duty_cycle) override
  { // Nova operação para controlar o PWM diretamente
    ledcWrite(LEDC_CHANNEL_PWM_HEATER, duty_cycle);
    Serial.printf("Callback: PWM Aquecedor - Duty Cycle: %d\n", duty_cycle);
  }

  /**
   * @brief Mapeia a temperatura alvo para o duty cycle do PWM e envia para controlHeaterPWM.
   * Chamado pelo Itemis (operação 'heat').
   * @param target_temp A temperatura alvo desejada (ex: 25C a 100C).
   */
  void heat(sc_integer target_temp) override
  { // Implementação da operação 'heat' do Itemis
    if (myStatechart != nullptr)
    {
      int minTemp = 25;                                                     // Temperatura mínima que o PWM representa 0%
      int maxTemp = 100;                                                    // Temperatura máxima que o PWM representa 100%
      int maxDutyCycle = (1 << myStatechart->getPwm_resolution_bits()) - 1; // Ex: 1023 para 10 bits

      int dutyCycle_calculated = 0;
      if (target_temp <= minTemp)
      {
        dutyCycle_calculated = 0;
      }
      else if (target_temp >= maxTemp)
      {
        dutyCycle_calculated = maxDutyCycle;
      }
      else
      {
        // Mapeamento linear: duty = (target - min) / (max - min) * maxDuty
        dutyCycle_calculated = map(target_temp, minTemp, maxTemp, 0, maxDutyCycle);
      }

      controlHeaterPWM(dutyCycle_calculated); // Chamar a função de controle de PWM diretamente
      Serial.printf("Callback: Operacao 'heat' chamada. Alvo Temp: %dC -> PWM Duty Calculado: %d (max %d)\n", target_temp, dutyCycle_calculated, maxDutyCycle);
    }
    else
    {
      Serial.println("Callback: ERRO: myStatechart é nullptr em heat().");
    }
  }

  // --- MÉTODO DE CALLBACK PARA O SENSOR DE ÁGUA ---
  /**
   * @brief Inicializa o sensor de temperatura DS18B20.
   * Chamado pelo estado INIT_SYSTEM do Itemis.
   */
  void beginWaterSensor() override
  {
    Serial.println("Callback: beginWaterSensor() chamado, mas nao inicializa mais sensor 1-Wire.");
    // A inicialização do I2C Master é feita no setup() do main.cpp
    // Nenhuma ação específica aqui para o sensor de temperatura simulado via I2C
  }

  // --- MÉTODOS DE CALLBACK PARA FUNÇÕES AINDA NÃO IMPLEMENTADAS OU SIMPLES ---
  void shutdownSystem() override {}
  void time(sc_integer) override {}
  void setTemperature(sc_integer) override {}
  void setTime(sc_integer) override {}
  void initializeSetupProcess() override {}

  // --- MÉTODOS DE CUSTOMIZAÇÃO (TBD) ---
  void showCustomSetup_GetNumSteps() override {}
  sc_boolean isValidNumSteps(sc_integer) override { return false; }
  void setNumCustomSteps(sc_integer) override {}
  void initializeStepDataCollection() override {}
  void showCustomSetup_PromptTemp(sc_integer) override {}
  void showCustomSetup_PromptTime(sc_integer) override {}
  sc_boolean isValidDataInput(sc_integer) override { return false; }
  void processTemperature(sc_integer, sc_integer) override {}
  void processDuration(sc_integer, sc_integer) override {}
  sc_boolean hasMoreStepsToDefine() override { return false; }
  void advanceToNextCustomStep() override {}
  void showCustomSetup_Summary() override {}

private:
  Statechart *myStatechart = nullptr; // Ponteiro para a instância da Statechart
};

#endif // STATECHARTCALLBACK_H