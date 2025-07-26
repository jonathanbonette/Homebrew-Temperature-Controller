/**
 * @file slave.ino
 * @brief Firmware de simulação para o sensor de temperatura I2C.
 * @details Este sketch configura um ESP32 para atuar como um dispositivo escravo (slave) em um barramento I2C.
 * Ele simula um sensor de temperatura, que é lido por um microcontrolador mestre (master). A lógica de simulação
 * calcula a temperatura baseada em um modelo de inércia térmica simples, onde a temperatura aumenta com a
 * potência de aquecimento (lida de um pino ADC) e diminui com o resfriamento natural para a temperatura ambiente.
 * @author Jonathan Chrysostomo Cabral Bonette
 * @date 26/07/2025
 * @note Este sketch é complementar ao `main.cpp` do projeto de brassagem, fornecendo uma leitura de temperatura
 * simulada que reage dinamicamente ao sinal de controle do mestre.
 * @copyright Copyright (c) 2025
 */

#include <Wire.h>

// --- DEFINES E VARIÁVEIS DE CONFIGURAÇÃO ---
/**
 * @brief Endereço I2C deste dispositivo escravo.
 * @details Deve ser o mesmo endereço configurado no microcontrolador mestre.
 */
constexpr uint8_t I2C_SLAVE_ADDRESS = 0x08; 

const int I2C_SDA_PIN = 21; // Pino SDA para a comunicação I2C.
const int I2C_SCL_PIN = 22; // Pino SCL para a comunicação I2C.

constexpr uint32_t I2C_FREQ = 100000;  // Frequência do barramento I2C em Hz (100kHz).

const int ADC_INPUT_PIN = 34; // Pino ADC utilizado para ler a simulação da potência de aquecimento (PWM).

// --- VARIÁVEIS DE SIMULAÇÃO DE TEMPERATURA ---
const float MIN_TEMP_SIMULATED = 25.0; 						// Temperatura mínima de simulação (correspondente a 0% PWM).
const float MAX_TEMP_SIMULATED = 100.0; 					// Temperatura máxima de simulação (correspondente a 100% PWM).
const int ADC_MAX_VALUE = 4095; 							// Valor máximo da leitura do ADC (12 bits de resolução).
volatile float simulatedTemperature = MIN_TEMP_SIMULATED; 	// Variável volátil para a temperatura simulada, pode ser acessada de uma ISR.
unsigned long last_simulated_temp_update_time; 				// Variável de tempo para calcular o delta t.

// --- PARÂMETROS DO MODELO DE INÉRCIA TÉRMICA ---
const float TEMPERATURA_AMBIENTE = 25.0; // Temperatura ambiente para o modelo de resfriamento.
const float GANHO_AQUECIMENTO = 1.0; // Fator de ganho para o aquecimento.
const float TAXA_RESFRIAMENTO = 0.05; // Fator de taxa de resfriamento para a temperatura ambiente.

// --- CALLBACKS I2C ---
/**
 * @brief Callback para a requisição de dados pelo mestre I2C.
 * @details Quando o mestre requisita dados deste escravo, esta função é chamada.
 * Ela copia a temperatura simulada para um array de bytes e a envia de volta ao mestre.
 */
void onRequest() {
  float temp_copy = simulatedTemperature; 
  Serial.printf("RequestEvent: Mestre requisitou dados. Enviando temperatura: %.2f C\n", temp_copy);
  byte data[4];
  memcpy(data, &temp_copy, 4); 
  Wire.write(data, 4); 
}

/**
 * @brief Callback para o recebimento de dados do mestre I2C.
 * @details Esta função é chamada quando o mestre envia dados para o escravo.
 * Atualmente, ela apenas imprime os dados recebidos para depuração.
 * @param howMany O número de bytes recebidos.
 */
void onReceive(int howMany) {
  Serial.printf("ReceiveEvent: Mestre enviou %d bytes. ", howMany);
  while (Wire.available()) {
    byte c = Wire.read(); 
    Serial.print((char)c);
  }
  Serial.println();
}

// --- ARDUINO SKETCH FUNCTIONS ---
/**
 * @brief Função de inicialização do sistema.
 * @details Configura a comunicação serial, o ADC e inicializa o ESP32 como
 * um dispositivo escravo I2C no endereço e pinos definidos.
 * Em seguida, configura os callbacks para os eventos de requisição e recebimento.
 */
void setup() {
  Serial.begin(115200);
  delay(200); 
  Serial.println("\nESP32 I2C Slave Simulator - Iniciando...");

  analogReadResolution(12); // Define a resolução do ADC para 12 bits (0-4095)

  // Inicializa o ESP32 como slave I2C
  bool ok = Wire.begin(I2C_SLAVE_ADDRESS, I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
  if (!ok) {
    Serial.println("Erro ao iniciar como slave I2C! Travando...");
    while (true) delay(1000);
  }

  Wire.onRequest(onRequest);    
  Wire.onReceive(onReceive);    
  
  Serial.println("Pronto como I2C slave no endereco 0x08.");
  Serial.printf("Pinos I2C: SDA=%d, SCL=%d. Frequencia: %d Hz.\n", I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
  Serial.printf("Pino ADC de entrada para simular aquecimento: GPIO%d\n", ADC_INPUT_PIN);

  last_simulated_temp_update_time = millis(); // Inicializa o tempo para cálculo do dt
}


/**
 * @brief Loop principal do programa.
 * @details Esta função é executada repetidamente para simular o comportamento
 * de um sensor de temperatura. Ela lê a entrada analógica (simulando a potência
 * de aquecimento), aplica um modelo de inércia térmica para calcular a nova
 * temperatura e atualiza a variável `simulatedTemperature`. O loop executa a
 * cada 100ms para uma simulação estável.
 */
void loop() {
  unsigned long current_time = millis();
  float dt_seconds = (float)(current_time - last_simulated_temp_update_time) / 1000.0; // Tempo em segundos
  last_simulated_temp_update_time = current_time;


  // Lê a potência do aquecedor (do PWM do mestre)
  int adc_value = analogRead(ADC_INPUT_PIN);
  float heating_power_0_to_1 = (float)adc_value / ADC_MAX_VALUE; // Converte ADC para escala 0.0 a 1.0

  float delta_T = 0; // Calcula a variação da temperatura (modelo de inércia)
  
  // Efeito do Aquecimento: Direto da potência do PWM
  delta_T += heating_power_0_to_1 * GANHO_AQUECIMENTO * dt_seconds; // heating_power_0_to_1 * GANHO_AQUECIMENTO => representa a taxa de aquecimento instantânea em C/s

  // Efeito do Resfriamento para o Ambiente: Se o tanque está mais quente/frio que o ambiente, ele tende ao ambiente
  delta_T -= (simulatedTemperature - TEMPERATURA_AMBIENTE) * TAXA_RESFRIAMENTO * dt_seconds;
  
  // Atualiza a temperatura simulada
  simulatedTemperature += delta_T;

  // Limita a temperatura simulada aos limites (bom para evitar valores absurdos)
  if (simulatedTemperature < MIN_TEMP_SIMULATED) simulatedTemperature = MIN_TEMP_SIMULATED;
  if (simulatedTemperature > MAX_TEMP_SIMULATED) simulatedTemperature = MAX_TEMP_SIMULATED; 

  // Imprimir a temperatura simulada para depuração no escravo
  Serial.printf("Slave: ADC = %d (P=%.2f), dT=%.3f, Temp=%.2f C\n", adc_value, heating_power_0_to_1, delta_T, simulatedTemperature);
  
  delay(100); // Roda o loop a cada 100ms
}