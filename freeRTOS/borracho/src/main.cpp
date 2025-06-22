#include <Arduino.h>
#include "src-gen/Statechart.h"
#include "StatechartCallback.h"
#include "StatechartTimer.h"

// FreeRTOS Headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// --- OBJETOS GLOBAIS ---
Statechart statechart;          ///< Instância da máquina de estados Yakindu.
StatechartCallback callback;    ///< Instância da classe de callbacks para operações do Yakindu.
StatechartTimer timerService;   ///< Instância do serviço de timer para a máquina de estados.

// Filas FreeRTOS para comunicação entre tarefas
QueueHandle_t xKeypadQueue;     ///< Fila para enviar teclas lidas da keypadTask para a stateMachineTask.
QueueHandle_t xDisplayQueue;    ///< Fila para enviar comandos de exibição para a displayTask.

// Objeto para o display OLED (acessível globalmente para a displayTask)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- PROTÓTIPOS DAS FUNÇÕES DAS TAREFAS ---
void keypadTask(void *pvParameters);
void stateMachineTask(void *pvParameters);
void displayTask(void *pvParameters);

// --- SETUP ARDUINO ---
/**
 * @brief Função de inicialização do sistema.
 * Configura a comunicação serial, I2C, inicializa a máquina de estados
 * e cria as tarefas FreeRTOS.
 */
void setup() {
  Serial.begin(115200);
  delay(1000); // Pequeno delay para estabilização
  Serial.println("Main: Iniciando FreeRTOS Setup...");

  Wire.begin(21, 22); // Inicializa I2C (SDA=21, SCL=22 para ESP32 DevKitC)

  // Configura a máquina de estados com seus serviços e callbacks
  statechart.setOperationCallback(&callback);
  statechart.setTimerService(&timerService);
  callback.setStatechart(&statechart); // Passa a referência da statechart para o callback

  // Cria as filas FreeRTOS
  xKeypadQueue = xQueueCreate(5, sizeof(char)); // Fila para 5 caracteres do teclado
  xDisplayQueue = xQueueCreate(10, sizeof(DisplayCommand)); // Fila para 10 comandos de display

  // Verifica se as filas foram criadas com sucesso
  if (xKeypadQueue == NULL || xDisplayQueue == NULL) {
    Serial.println("Main: ERRO! Falha ao criar filas FreeRTOS. Sistema parado.");
    for(;;); // Loop infinito em caso de erro crítico
  }

  // Cria as tarefas FreeRTOS com suas prioridades e tamanhos de pilha
  // Prioridades: Display > Keypad/StateMachine (para UI responsiva)
  xTaskCreate(keypadTask,       "KeypadTask",       2048, NULL, 1, NULL);
  xTaskCreate(displayTask,      "DisplayTask",      4096, NULL, 2, NULL);
  xTaskCreate(stateMachineTask, "StateMachineTask", 4096, NULL, 1, NULL);

  // Inicia a máquina de estados (entra no estado inicial definido no modelo Yakindu)
  statechart.enter();
}

// --- LOOP ARDUINO ---
/**
 * @brief Loop principal do Arduino.
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

  callback.beginMatrix(); // Inicializa o hardware do teclado matricial

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

      // Atualiza o buffer de entrada do teclado no callback
      callback.inputBuffer += receivedKey;
      callback.lastKeyPressTime = millis(); // Timestamp da última tecla (para timeout)

      // --- Lógica de Decisão e Disparo de Eventos Yakindu ---
      // Ações são tomadas com base no estado atual da máquina de estados.

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
              case '1': statechart.raiseRecipe_1_process(); callback.inputBuffer = ""; break;
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
              case '1': statechart.raiseRecipe_2_process(); callback.inputBuffer = ""; break;
              case '2': statechart.raiseRecipe_back_menu(); callback.inputBuffer = ""; break;
              default:
                callback.showRecipe(2); // Passa '2' para showRecipeDetailsScreen
                callback.printKeypadInput();
                break;
          }
      }
      else if (statechart.isStateActive(Statechart::main_region_RECIPE_3)) {
          switch (receivedKey) {
              case '1': statechart.raiseRecipe_3_process(); callback.inputBuffer = ""; break;
              case '2': statechart.raiseRecipe_back_menu(); callback.inputBuffer = ""; break;
              default:
                callback.showRecipe(3); // Passa '3' para showRecipeDetailsScreen
                callback.printKeypadInput();
                break;
          }
      }
      else if (statechart.isStateActive(Statechart::main_region_RECIPE_4)) {
          switch (receivedKey) {
              case '1': statechart.raiseRecipe_4_process(); callback.inputBuffer = ""; break;
              case '2': statechart.raiseRecipe_back_menu(); callback.inputBuffer = ""; break;
              default:
                callback.showRecipe(4); // Passa '4' para showRecipeDetailsScreen
                callback.printKeypadInput();
                break;
          }
      }
      // TODO: Adicionar o else if para Statechart::main_region_RECIPE_5 (Customizar) quando for implementado.
      // E também para o estado CUSTOM_SETUP (se for um menu com entrada de dados).

      // Lógica para estados onde a entrada de teclado é inesperada (ex: durante um processo de aquecimento)
      else {
        switch (receivedKey) {
          case 'A': // Assumindo 'A' como um botão universal para voltar ao MENU principal de receitas
            statechart.raiseMenu(); // Volta para o menu de receitas
            callback.inputBuffer = ""; // Limpa o buffer ao mudar de tela
            break;
          // TODO: Adicionar outros botões de função global aqui (ex: Parar 'B', Reset '#')
          default:
            // Para qualquer outra tecla que não 'A' e não esperada no estado atual:
            // Apenas limpa o buffer de entrada. Não redesenha a tela
            // para evitar interromper uma tela de processo/progresso.
            callback.inputBuffer = "";
            Serial.println("StateMachineTask: Tecla ignorada no estado atual.");
            break;
        }
      }
    } // Fim de if (xQueueReceive...)

    // Lógica de limpeza do inputBuffer após um timeout (se o usuário parar de digitar)
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
      }
      // TODO: Adicionar outros estados relevantes aqui.
    }
  } // Fim do loop for(;;)
}

/**
 * @brief Tarefa responsável por gerenciar todas as operações de exibição no display OLED.
 * Recebe comandos da xDisplayQueue e desenha o conteúdo correspondente.
 */
void displayTask(void *pvParameters) {
  (void) pvParameters; // Evita warning de parâmetro não utilizado

  // Inicialização REAL e única do display OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("DisplayTask: ERRO! Falha ao inicializar o display. Sistema parado."));
    for(;;); // Loop infinito em caso de erro crítico
  }
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("DisplayTask OK!");
  display.display();
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Breve delay para mostrar a mensagem inicial

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