 /**
 * @file main.cpp
 * @brief Ponto de entrada principal do firmware para o sistema de brassagem de cerveja.
 * @details Este arquivo configura o ambiente FreeRTOS, a máquina de estados Yakindu,
 * o display OLED, o controlador PID e outras periféricos. Ele cria e gerencia
 * as tarefas (tasks) que executam a lógica principal do sistema, como leitura
 * do teclado, controle da máquina de estados, exibição no display e controle
 * do processo de aquecimento.
 * @author Seu Nome
 * @date 26/07/2025
 * @note Este firmware utiliza FreeRTOS para gerenciamento de tarefas, a biblioteca
 * Yakindu para a máquina de estados e o protocolo I2C para comunicação com
 * sensores externos.
 * @copyright Copyright (c) 2025
 */

#include <Arduino.h>
#include "src-gen/Statechart.h"
#include "StatechartCallback.h"
#include "StatechartTimer.h"

// FreeRTOS Headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Para teste com Slave
#include <Wire.h>

// PID
#include <PID_v1.h>

// --- OBJETOS GLOBAIS ---
/**
 * @brief Instâncias e recursos globais compartilhados entre as tarefas.
 * @details Esta seção define os objetos principais do sistema, como a
 * máquina de estados, seus callbacks, serviços de timer e as filas
 * de comunicação FreeRTOS.
 */
Statechart statechart;          ///< Instância da máquina de estados Yakindu
StatechartCallback callback;    ///< Instância da classe de callbacks para operações do Yakindu
StatechartTimer timerService;   ///< Instância do serviço de timer para a máquina de estados

// Filas FreeRTOS para comunicação entre tarefas
QueueHandle_t xKeypadQueue;     ///< Fila para enviar teclas lidas da keypadTask para a stateMachineTask
QueueHandle_t xDisplayQueue;    ///< Fila para enviar comandos de exibição para a displayTask
QueueHandle_t xControlQueue;    ///< Fila para comandos da controlTask
QueueHandle_t xSensorQueue;     ///< Fila para leitura do sensor de temperatura

// Objeto para o display OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- ENDEREÇO I2C DO SIMULADOR DE SENSOR ---
/**
 * @brief Endereço I2C do ESP32 escravo (simulador de sensor).
 * @details Endereço I2C escolhido para o ESP32 que simula o sensor de temperatura.
 * Este endereço deve ser único na rede I2C.
 */
const byte I2C_SLAVE_ADDRESS = 0x08;

// --- PROTÓTIPOS DAS FUNÇÕES DAS TAREFAS ---
void keypadTask(void *pvParameters);
void stateMachineTask(void *pvParameters);
void displayTask(void *pvParameters);
void controlTask(void *pvParameters);
void temperatureSensorTask(void *pvParameters); 

// --- VARIÁVEIS PID ---
// Define as entradas, saídas e setpoints para o PID
double Setpoint, Input, Output; // Usa double para a lib do PID_v1
double Kp=30, Ki=5.0, Kd=0.5; // Coeficientes PID iniciais (Inicial: 10, 0.1, 0.5)

// Especifica o objeto PID
// PID(&Input, &Output, &Setpoint, Kp, Ki, Kd, Direction)
// Direction: DIRECT (Output aumenta quando Input diminui em relação ao Setpoint)
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

// --- SETUP ---
/**
 * @brief Função de inicialização do sistema.
 * Configura a comunicação serial, I2C, inicializa a máquina de estados
 * e cria as tarefas FreeRTOS.
 */
void setup() {
  Serial.begin(115200);
  delay(1000); // Pequeno delay para estabilização
  Serial.println("Main: Iniciando FreeRTOS Setup...");

 // 1. Inicialização do OLED (crucial, mas se der erro, pode travar o setup)
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Main: ERRO! Falha ao inicializar o display no setup. Sistema parado."));
    for(;;); // Trava aqui se o display for vital para o debug
  } else {
    display.setTextSize(1); display.setTextColor(SSD1306_WHITE); display.setCursor(0,0);
    display.println("Main: Display OK!"); display.display(); delay(500);
  }

  // --- INICIALIZAÇÕES DE HARDWARE ---
  // Inicializa I2C como MESTRE
  Wire.begin(21, 22);
  Wire.setClock(100000); // Define a frequência para 100kHz
  Serial.println("Main: I2C Master inicializado nos pinos 21 (SDA) e 22 (SCL).");

  // Configura a máquina de estados com seus serviços e callbacks
  statechart.setOperationCallback(&callback);
  statechart.setTimerService(&timerService);
  callback.setStatechart(&statechart); // Passa a referência da statechart para o callback

  // GARANTE A INICIALIZAÇÃO DO PWM!
  callback.setupHeaterPWM();

  // --- CONFIGURAÇÃO DO PID ---
  // Define os limites de saída do PID para o duty cycle do PWM (0 a 1023 para 10 bits)
  myPID.SetOutputLimits(0, (double)((1 << statechart.getPwm_resolution_bits()) - 1)); // Max duty cycle
  myPID.SetMode(AUTOMATIC); // Inicia o PID no modo automático (ligado)
  myPID.SetSampleTime(100); // Define o tempo de amostragem do PID em milissegundos (o mesmo do loop da controlTask)
  Serial.println("Main: Controlador PID inicializado.");


  // Cria as filas FreeRTOS
  xKeypadQueue = xQueueCreate(5, sizeof(char)); // Fila para 5 caracteres do teclado
  xDisplayQueue = xQueueCreate(10, sizeof(DisplayCommand)); // Fila para 10 comandos de display
  xControlQueue = xQueueCreate(5, sizeof(ControlCommand)); // Fila para controles de seleção
  xSensorQueue = xQueueCreate(1, sizeof(TemperatureData)); // Fila para controle de temperatura
  
  // Verifica se as filas foram criadas com sucesso
  if (xKeypadQueue == NULL || xDisplayQueue == NULL || xControlQueue == NULL || xSensorQueue == NULL) {
    Serial.println("Main: ERRO! Falha ao criar filas FreeRTOS. Sistema parado.");
    for(;;); // Loop infinito em caso de erro crítico
  }

  // Cria as tarefas FreeRTOS com suas prioridades e tamanhos de pilha
  // Prioridades: Display > Keypad/StateMachine
  xTaskCreate(keypadTask,       "KeypadTask",       2048, NULL, 1, NULL);
  xTaskCreate(displayTask,      "DisplayTask",      4096, NULL, 2, NULL);
  xTaskCreate(stateMachineTask, "StateMachineTask", 4096, NULL, 1, NULL);
  xTaskCreate(controlTask,      "ControlTask",      4096, NULL, 1, NULL);
  xTaskCreate(temperatureSensorTask, "TempSensorTask", 2048, NULL, 1, NULL);

  // Inicia a máquina de estados (entra no estado inicial definido no modelo Yakindu)
  statechart.enter();
}

// --- LOOP ---
/**
 * @brief Loop principal do FreeRTOS.
 * No contexto FreeRTOS, esta função geralmente contém um delay
 * ou é usada para uma tarefa de baixa prioridade, pois a maioria
 * da lógica é executada pelas tasks criadas.
 */
void loop() {
  vTaskDelay(10 / portTICK_PERIOD_MS); // Pequeno delay para permitir que o scheduler execute outras tasks
}

// --- IMPLEMENTAÇÃO DAS TAREFAS FREE RTOS ---
/**
 * @brief Tarefa responsável pela leitura contínua do teclado matricial.
 * Envia as teclas pressionadas para a fila do teclado (xKeypadQueue).
 */
void keypadTask(void *pvParameters) {
  (void) pvParameters; // Evita warning de parâmetro não utilizado

  char key;
  for (;;) { // Loop infinito da tarefa
    key = callback.readKeypadChar(); // Tenta ler uma tecla
    if (key != NO_KEY) { // Se uma tecla foi pressionada
      xQueueSend(xKeypadQueue, &key, portMAX_DELAY); // Envia a tecla para a fila (espera indefinidamente se cheia)
      Serial.print("KeypadTask: Tecla enviada para fila: ");
      Serial.println(key);
    }
    vTaskDelay(50 / portTICK_PERIOD_MS); // Pequeno delay para debounce e liberar CPU
  }
}

/**
 * @brief Tarefa responsável por gerenciar a máquina de estados Yakindu
 * e processar a entrada do teclado.
 */
void stateMachineTask(void *pvParameters) {
  (void) pvParameters; // Evita warning de parâmetro não utilizado

  char receivedKey;
  for (;;) { // Loop infinito da tarefa
    // Espera por uma tecla na fila (bloqueia até uma tecla ser recebida)
    if (xQueueReceive(xKeypadQueue, &receivedKey, portMAX_DELAY) == pdPASS) {
      Serial.print("StateMachineTask: Tecla recebida: ");
      Serial.println(receivedKey);

      ControlCommand controlCmd; // Inicializa os comandos fora do escopo das funções

      // Atualiza o buffer de entrada do teclado no callback
      callback.inputBuffer += receivedKey;
      callback.lastKeyPressTime = millis(); // Timestamp da última tecla (para timeout)

      // --- LÓGICA DE DECISÃO E DISPARO DE EVENTOS DO STATECHART/YAKINDU/ITEMIS ---
      // Ações são tomadas com base no estado atual da máquina de estados

      // Estado IDLE (tela inicial de "Bem-vindo")
      if (statechart.isStateActive(Statechart::main_region_IDLE)) {
        switch (receivedKey) {
          case '1': statechart.raiseStart_button(); callback.inputBuffer = ""; break;
          case '2': statechart.raiseExit_process(); callback.inputBuffer = ""; break;
          default:
            // Tecla inválida em IDLE: Apenas redesenha a tela IDLE com o input atualizado
            callback.showIdleScreen(); // Redesenha a tela "Bem-vindo"
            callback.printKeypadInput(); // Imprime o "Digitado: " na tela
            break;
        }
      }
      // Estado MENU (tela de seleção de receitas)
      else if (statechart.isStateActive(Statechart::main_region_MENU)) {
        switch (receivedKey) {
          case '1': statechart.raiseRecipe_1(); callback.inputBuffer = ""; break;
          case '2': statechart.raiseRecipe_2(); callback.inputBuffer = ""; break;
          case '3': statechart.raiseRecipe_3(); callback.inputBuffer = ""; break;
          case '4': statechart.raiseRecipe_4(); callback.inputBuffer = ""; break;
          case '5': statechart.raiseRecipe_5(); callback.inputBuffer = ""; break;
          default:
            // Tecla inválida em MENU: Redesenha o menu de receitas com o input atualizado
            callback.showRecipes(); // Redesenha a lista de receitas
            callback.printKeypadInput(); // Imprime o "Digitado: " na tela
            break;
        }
      }
      // Estados de DETALHES de Receitas (RECIPE_1 a RECIPE_4)
      // Cada estado de detalhes de receita tem sua própria lógica de botões '1' (Iniciar) e '2' (Voltar)
      else if (statechart.isStateActive(Statechart::main_region_RECIPE_1)) {
          switch (receivedKey) {
              case '1':
                // Define qual receita será processada e dispara o início do processo
                callback.currentRecipeIdx = 0; // American Pale Ale é índice 0
                statechart.raiseRecipe_1_process();
                statechart.raiseStart_first_step(); // Dispara a primeira transição dentro de STANDARD_PROCESS
                callback.inputBuffer = "";
                break;
              case '2': statechart.raiseRecipe_back_menu(); callback.inputBuffer = ""; break;
              default:
                // Tecla inválida em RECIPE_1: Redesenha os detalhes da receita 1 com o input
                callback.showRecipe(1); // Passa '1' para showRecipeDetailsScreen
                callback.printKeypadInput(); // Imprime o "Digitado: " na tela
                break;
          }
      }
      else if (statechart.isStateActive(Statechart::main_region_RECIPE_2)) {
          switch (receivedKey) {
              case '1':
                callback.currentRecipeIdx = 1; // Witbier é índice 1
                statechart.raiseRecipe_2_process();
                statechart.raiseStart_first_step();
                callback.inputBuffer = "";
                break;
              case '2': statechart.raiseRecipe_back_menu(); callback.inputBuffer = ""; break;
              default:
                callback.showRecipe(2); // Passa '2' para showRecipeDetailsScreen
                callback.printKeypadInput();
                break;
          }
      }
      else if (statechart.isStateActive(Statechart::main_region_RECIPE_3)) {
          switch (receivedKey) {
              case '1':
                callback.currentRecipeIdx = 2; // Belgian Dubbel é índice 2
                statechart.raiseRecipe_3_process();
                statechart.raiseStart_first_step();
                callback.inputBuffer = "";
                break;
              case '2': statechart.raiseRecipe_back_menu(); callback.inputBuffer = ""; break;
              default:
                callback.showRecipe(3); // Passa '3' para showRecipeDetailsScreen
                callback.printKeypadInput();
                break;
          }
      }
      else if (statechart.isStateActive(Statechart::main_region_RECIPE_4)) {
          switch (receivedKey) {
              case '1':
                callback.currentRecipeIdx = 3; // Bohemian Pilsen é índice 3
                statechart.raiseRecipe_4_process();
                statechart.raiseStart_first_step();
                callback.inputBuffer = "";
                break;
              case '2': statechart.raiseRecipe_back_menu(); callback.inputBuffer = ""; break;
              default:
                callback.showRecipe(4); // Passa '4' para showRecipeDetailsScreen
                callback.printKeypadInput();
                break;
          }
      }
      // Lógica para o estado FINISHED_MESSAGE
      else if (statechart.isStateActive(Statechart::main_region_FINISHED_MESSAGE)) {
          // Neste estado, não esperamos entrada de teclado para navegação,
          // apenas um timeout fará a transição para IDLE. Definido no Yakindu
          // Ignora qualquer tecla pressionada para evitar interferência na mensagem
          callback.inputBuffer = ""; // Sempre limpa o buffer
          Serial.println("StateMachineTask: Tecla ignorada no estado FINISHED_MESSAGE.");
      }

      // TODO: Adicionar o else if para Statechart::main_region_RECIPE_5 (Customizar) quando for implementado
      // E também para o estado CUSTOM_SETUP (se for um menu com entrada de dados)

      // Lógica para estados onde a entrada de teclado é inesperada (ex: durante um processo de aquecimento)
      else {
        switch (receivedKey) {
          case 'A': // Assumindo 'A' como um botão universal para voltar ao MENU principal de receitas
            // TODO: Aqui precisaria de uma lógica mais robusta para "abortar" um processo
            // que está rodando. Isso envolveria desligar os atuadores, etc., antes de mudar de estado
            // ENVIA COMANDO PARA ABORTAR O PROCESSO!
            controlCmd.type = CMD_START_RECIPE_STEP;
            xQueueSend(xControlQueue, &controlCmd, portMAX_DELAY); // Sinaliza para controlTask abortar

            // Depois de sinalizar, a StateMachine pode transitar para IDLE ou um estado de Erro/Abortado
            statechart.raiseMenu(); // Volta para o menu de receitas
            callback.inputBuffer = "";
            Serial.println("StateMachineTask: Tecla 'A' para voltar ao menu (ABORT).");
            break;
          // TODO: Adicionar outros botões de função global aqui (ex: Parar 'B', Reset '#')
          default:
            // Para qualquer outra tecla que não 'A' e não esperada no estado atual:
            // Apenas limpa o buffer de entrada. Não redesenha a tela
            // para evitar interromper uma tela de processo/progresso
            callback.inputBuffer = "";
            Serial.println("StateMachineTask: Tecla ignorada no estado atual.");
            break;
        }
      }
    }

    // Lógica de limpeza do inputBuffer após um timeout (se o usuário parar de digitar no keypad)
    if (callback.inputBuffer.length() > 0 && (millis() - callback.lastKeyPressTime > 3000)) {
      callback.inputBuffer = ""; // Limpa o buffer
      // Redesenha a tela atual para remover o "Digitado: " que estava aparecendo
      if (statechart.isStateActive(Statechart::main_region_IDLE)) {
        callback.showIdleScreen();
      } else if (statechart.isStateActive(Statechart::main_region_MENU)) {
        callback.showRecipes(); // Usa showRecipes() que já limpa e redesenha a lista
      } else if (statechart.isStateActive(Statechart::main_region_RECIPE_1)) {
        callback.showRecipe(1); // Usa showRecipe() que já limpa e redesenha os detalhes
      } else if (statechart.isStateActive(Statechart::main_region_RECIPE_2)) {
        callback.showRecipe(2);
      } else if (statechart.isStateActive(Statechart::main_region_RECIPE_3)) {
        callback.showRecipe(3);
      } else if (statechart.isStateActive(Statechart::main_region_RECIPE_4)) {
        callback.showRecipe(4);
      } else if (statechart.isStateActive(Statechart::main_region_RECIPE_5)) {
        callback.showRecipe(5); // Para Customizar
      }
      // Tratamento para o estado FINISHED_MESSAGE
      else if (statechart.isStateActive(Statechart::main_region_FINISHED_MESSAGE)) {
          // A tela FINISHED_MESSAGE já tem um timer para voltar, não precisa redesenhar por timeout
          // Se o usuário digitou algo, apenas limpa o buffer sem redesenhar
      }
      // TODO: Adicionar outros estados se tiver aqui
    }
  } // Fim do loop for(;;)
}

/**
 * @brief Tarefa responsável por gerenciar todas as operações de exibição no display OLED.
 * Recebe comandos da xDisplayQueue e desenha o conteúdo correspondente.
 */
void displayTask(void *pvParameters) {
  (void) pvParameters; // Evita warning de parâmetro não utilizado

  DisplayCommand cmd;
  for (;;) { // Loop infinito da tarefa
    // Espera por comandos na fila do display (bloqueia até um comando ser recebido)
    if (xQueueReceive(xDisplayQueue, &cmd, portMAX_DELAY) == pdPASS) {
      if (cmd.clearScreen) { // Limpa a tela apenas se o comando exigir
        display.clearDisplay();
      }
      display.setCursor(0, 0); // Reinicia o cursor no topo esquerdo para a maioria dos desenhos

      // Processa o tipo de comando recebido
      switch (cmd.type) {
        case CMD_CLEAR_DISPLAY:
          display.clearDisplay(); // Limpeza explícita
          break;
        case CMD_SHOW_STATE_INFO:
          // Desenha o nome do estado no topo, sem limpar o resto da tela
          display.fillRect(0, 0, SCREEN_WIDTH, 8, SSD1306_BLACK); // Limpa a linha superior
          display.setCursor(0, 0);
          display.print("Estado: ");
          display.println(cmd.text);
          break;
        case CMD_SHOW_MAIN_MENU_SCREEN: // Tela IDLE: "Bem-vindo", "1-Iniciar", "2-Sair"
          display.println("Bem-vindo!");
          display.setCursor(0, 16);
          display.println("1 - Iniciar");
          display.println("2 - Sair");
          display.println("Pressione A");
          break;
        case CMD_SHOW_STARTUP_MESSAGE: // Mensagem de inicialização (apenas texto)
          display.println("Executando showStartup()");
          break;
        case CMD_SHOW_RECIPES_LIST: // Menu de seleção de receitas
          display.println("Receitas:");
          display.setCursor(0, 16);
          display.println("1- American Pale Ale");
          display.println("2- Witbier");
          display.println("3- Belgian Dubbel");
          display.println("4- Bohemian Pilsen");
          display.println("5- Customizar");
          break;
        case CMD_SHOW_RECIPE_DETAILS_SCREEN: { // Detalhes de uma receita específica
          if (cmd.recipeId >= 0 && cmd.recipeId < NUM_RECIPES) {
            const Recipe& currentRecipe = recipes[cmd.recipeId];

            display.println(currentRecipe.name);
            display.print("Etapas: ");
            display.println(currentRecipe.numSteps);
            display.println(); // Pula uma linha

            int yPos = 32; // Posição Y inicial para as etapas
            for (int i = 0; i < currentRecipe.numSteps; ++i) {
              if (yPos + 8 > SCREEN_HEIGHT - 16) { // Verifica se há espaço antes de imprimir
                  // Se não houver espaço para mais etapas, podemos indicar que há mais
                  display.setCursor(0, yPos);
                  display.println("...mais etapas");
                  break; // Sai do loop para não estourar a tela
              }
              display.setCursor(0, yPos);
              display.print("* ");
              display.print(currentRecipe.steps[i].name);
              display.print(" ");
              display.print(currentRecipe.steps[i].temperature);
              display.print(" C ");
              display.print(currentRecipe.steps[i].duration);
              display.println(" min");
              yPos += 8; // Avança para a próxima linha
            }
            display.println(); // Pula uma linha
            // Posiciona as opções de iniciar/voltar no final da tela
            display.setCursor(0, SCREEN_HEIGHT - 16); // 2 linhas de 8 pixels cada
            display.println("1 - Iniciar Receita");
            display.println("2 - Voltar as Receitas");

          } else {
            display.println("ERRO: Receita invalida!");
          }
          break; // Fim do case CMD_SHOW_RECIPE_DETAILS_SCREEN
        }
        // CASE para exibir o status do processo
        case CMD_SHOW_PROCESS_STATUS_SCREEN:
          display.println("Processo Ativo:");
          display.setCursor(0, 16); // Posiciona abaixo do título
          display.println(cmd.text); // Imprime a string de status preparada
          break;
        // CASE para exibir mensagem de conclusão
        case CMD_SHOW_FINISHED_MESSAGE_SCREEN:
          display.println("Processo Concluido!");
          display.setCursor(0, 16);
          display.println("Receita finalizada.");
          display.setCursor(0, 32);
          display.println("Voltando ao menu principal...");
          break;
        case CMD_PRINT_KEYPAD_INPUT: // Imprime o texto digitado pelo teclado
          // Localiza a posição para o texto digitado (geralmente no rodapé)
          display.fillRect(0, SCREEN_HEIGHT - 8, SCREEN_WIDTH, 8, SSD1306_BLACK); // Limpa a última linha
          display.setCursor(0, SCREEN_HEIGHT - 8); // Última linha do display
          display.println(cmd.text);
          break;
      }
      display.display(); // Atualiza o display físico com todas as alterações
    }
    // A tarefa não precisa de delay explícito aqui se está esperando em xQueueReceive com portMAX_DELAY
  }
}

/**
 * @brief Tarefa responsável por gerenciar o processo de cozimento
 * (contagem regressiva e leitura dos dados do sensor de temperatura/atuadores).
 */
void controlTask(void *pvParameters) {
  (void) pvParameters;

  ControlCommand receivedControlCmd;
  TemperatureData currentSensorTempData;
  int currentTargetTemp = 0;
  int currentDurationMinutes = 0;
  unsigned long stepStartTimeMillis = 0; // Tempo em que a contagem "real" da etapa começa
  bool stepActive = false;
  float actualCurrentTemp = 0.0; // A temperatura atual real

  int activeRecipeIdx = -1;
  int activeStepIdx = -1;

  // Flag para indicar se o Setpoint foi atingido para iniciar a contagem regressiva da etapa
  bool setpointReachedForTiming = false; 

  for (;;) {
    // --- Processar Comandos da Fila de Controle (xControlQueue) ---
    if (xQueueReceive(xControlQueue, &receivedControlCmd, 0) == pdPASS) {
      switch (receivedControlCmd.type) {
        case CMD_START_RECIPE_STEP:
          activeRecipeIdx = receivedControlCmd.recipeIndex;
          activeStepIdx = receivedControlCmd.stepIndex;
          
          if (activeRecipeIdx >= 0 && activeRecipeIdx < NUM_RECIPES) {
            const Recipe& currentRecipeData = recipes[activeRecipeIdx];
            if (activeStepIdx >= 0 && activeStepIdx < currentRecipeData.numSteps) {
              currentTargetTemp = receivedControlCmd.targetTemperature;
              currentDurationMinutes = receivedControlCmd.durationMinutes;
              
              stepActive = true;

              Setpoint = (double)currentTargetTemp; 
              myPID.SetMode(AUTOMATIC); // Ativa o PID para controlar a temperatura

              setpointReachedForTiming = false; // Reseta a flag para a nova etapa
              
              Serial.printf("ControlTask: INICIADA ETAPA '%s'. Alvo: %dC, Duracao: %dmin\n",
                            (activeRecipeIdx == 4 ? "Customizada" : recipes[activeRecipeIdx].steps[activeStepIdx].name.c_str()), currentTargetTemp, currentDurationMinutes);
            }
          }
          break;
        case CMD_ABORT_PROCESS:
          Serial.println("ControlTask: Processo ABORTADO por comando.");
          stepActive = false;
          myPID.SetMode(MANUAL); // Desliga o PID
          Output = 0; // Força a saída do PID para 0
          callback.controlHeaterPWM(0); // Garante o PWM desligado
          setpointReachedForTiming = false; // Reseta a flag em caso de aborto
          break;
      }
    }

    // --- Receber Última Temperatura do Sensor (xSensorQueue) ---
    if (xQueueReceive(xSensorQueue, &currentSensorTempData, 0) == pdPASS) {
        actualCurrentTemp = currentSensorTempData.temperature1;
        Input = (double)actualCurrentTemp; // Atualiza o Input do PID
    }
    
    // --- Lógica de Controle/Monitoramento da Etapa ---
    if (stepActive) { // SE O PROCESSO ESTÁ ATIVO

      // --- LÓGICA DE INÍCIO DA CONTAGEM DO TEMPO ---
      // Se a temperatura alvo ainda não foi atingida para o timing, verifique se ela foi agora
      if (!setpointReachedForTiming) {
          float band_tolerance = 1.0; // Tolerância de 1 grau C (ex: entre 66C e 68C para alvo 67C)
          if (actualCurrentTemp >= (currentTargetTemp - band_tolerance) && actualCurrentTemp <= (currentTargetTemp + band_tolerance)) {
              setpointReachedForTiming = true;
              stepStartTimeMillis = millis(); // INICIA A CONTAGEM AQUI!
              Serial.printf("ControlTask: Setpoint %dC atingido! Iniciando contagem de %d minutos.\n", currentTargetTemp, currentDurationMinutes);
          }
      }

      // Calcula o Tempo Restante: Isso só faz a contagem regressiva se o setpoint foi atingido
      // Caso contrário, ele mantém o valor total da duração para exibição ("Aguardando...")
      int remainingTimeSeconds; 
      int displayMinutes;
      int displaySeconds;

      if (setpointReachedForTiming) {
          unsigned long elapsedTimeMillis = millis() - stepStartTimeMillis;
          remainingTimeSeconds = (currentDurationMinutes * 60) - (elapsedTimeMillis / 1000);
          if (remainingTimeSeconds < 0) remainingTimeSeconds = 0; // Não deixa tempo negativo

          displayMinutes = remainingTimeSeconds / 60;
          displaySeconds = remainingTimeSeconds % 60;
      } else { 
          // Se ainda está em fase de rampa, exibe a duração total como "tempo restante"
          remainingTimeSeconds = currentDurationMinutes * 60; 
          displayMinutes = remainingTimeSeconds / 60;
          displaySeconds = remainingTimeSeconds % 60;
      }
      
      // --- CALCULA E APLICA O PID ---
      myPID.Compute(); // Calcula o PID e atualiza a variável 'Output'
      int calculated_duty_cycle = (int)Output; 
      callback.controlHeaterPWM(calculated_duty_cycle); // Aplica o duty cycle calculado pelo PID

      // Atualizar Display de Status Periodicamente (a cada 1 segundo)
      static unsigned long lastDisplayUpdate = 0;
      if (millis() - lastDisplayUpdate >= 1000) { // Usar >= para garantir que não perca o tick
        lastDisplayUpdate = millis();
        const Recipe& recipeForDisplay = recipes[activeRecipeIdx];
        const char* stepNameForDisplay;
        if (activeRecipeIdx == 4) { // Receita customizada
          char tempBuffer[20];
          sprintf(tempBuffer, "Etapa %d", activeStepIdx + 1);
          stepNameForDisplay = tempBuffer;
        } else {
          stepNameForDisplay = recipeForDisplay.steps[activeStepIdx].name.c_str();
        }

        callback.showProcessStatus(
            static_cast<sc_integer>(actualCurrentTemp), 
            currentTargetTemp, 
            displayMinutes, 
            displaySeconds,
            const_cast<sc_string>(stepNameForDisplay), 
            activeStepIdx + 1, 
            (activeRecipeIdx == 4 ? statechart.getCustom_num_steps() : recipeForDisplay.numSteps),
            !setpointReachedForTiming // <--- A flag do isRamping == true se está na fase da rampa
        );
      }

      // --- Detecção de Término de Etapa ---
      // A etapa só termina SE O SETPOINT FOI ATINGIDO E O TEMPO CHEGOU A ZERO.
      if (setpointReachedForTiming && remainingTimeSeconds == 0) { 
        Serial.println("ControlTask: ETAPA CONCLUIDA! Disparando step_finished.");
        stepActive = false; // A etapa terminou
        myPID.SetMode(MANUAL); // Coloca o PID em modo manual
        Output = 0; // Força a saída do PID para 0
        callback.controlHeaterPWM(0); // Garante que o PWM seja 0
        // TODO: Desligar mixer se estiver ligado
        statechart.raiseStep_finished(); // Dispara o evento de fim de etapa
      }
    } else { // SE O PROCESSO NÃO ESTÁ ATIVO (ex: antes de iniciar, ou após terminar/abortar)
        // Garante que o aquecedor esteja desligado
        myPID.SetMode(MANUAL); // Certifica-se que o PID está desligado
        Output = 0; // Força a saída do PID para 0
        callback.controlHeaterPWM(0); // Garante que o PWM esteja desligado quando não tem etapa ativa
    }

    vTaskDelay(100 / portTICK_PERIOD_MS); // Roda o ciclo de controle a cada 100ms
  }
}

// --- TAREFA DO SENSOR DE TEMPERATURA: temperatureSensorTask ---
/**
 * @brief Tarefa responsável por ler periodicamente a temperatura do ESP32 simulador via I2C.
 * Envia as leituras para a xSensorQueue.
 */
void temperatureSensorTask(void *pvParameters) {
  (void) pvParameters;

  float tempC = -1000.0; // Variável para a temperatura lida
  TemperatureData tempDataToSend; // Estrutura para enviar via fila

  Serial.println("TempSensorTask: Iniciando leitura I2C do sensor simulado...");

  for (;;) {
    // Solicita 4 bytes (para um float) do ESP32 escravo no endereço I2C_SLAVE_ADDRESS
    Wire.requestFrom(I2C_SLAVE_ADDRESS, 4); // Solicita 4 bytes (para um float)

    if (Wire.available()) {
      // Receber os 4 bytes e reconstruir o float
      byte i2c_data[4];
      for (int i = 0; i < 4; i++) {
        i2c_data[i] = Wire.read();
      }
      // Reinterpretar os bytes como um float
      memcpy(&tempC, i2c_data, 4);

      tempDataToSend.temperature1 = tempC;
      xQueueOverwrite(xSensorQueue, &tempDataToSend); // Envia para a fila do sensor
      Serial.printf("TempSensorTask: Leitura I2C recebida: %.2f C\n", tempC);
    } else {
      Serial.println("TempSensorTask: ERRO! Nao ha bytes disponiveis para leitura I2C.");
      tempDataToSend.temperature1 = -999.0; // Sinaliza erro de leitura
      xQueueOverwrite(xSensorQueue, &tempDataToSend);
    }
    
    // Atraso entre leituras I2C. Ajustar conforme a necessidade.
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Lê a cada 1 segundo
  }
}