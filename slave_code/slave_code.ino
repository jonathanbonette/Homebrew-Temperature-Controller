#include <Wire.h>

// Define o endereço I2C para o ESP32
constexpr uint8_t I2C_SLAVE_ADDRESS = 0x08; 

// Pinos I2C do ESP32 (SDA, SCL) - Mesmos pinos do Mestre
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;

// Frequência do barramento I2C (100 kHz)
constexpr uint32_t I2C_FREQ = 100000; 

// Pino ADC que vai receber o sinal do PWM do mestre (GPIO34)
const int ADC_INPUT_PIN = 34;

// Valores de simulação de temperatura e ADC
const float MIN_TEMP_SIMULATED = 25.0; // Corresponde a 0% PWM / 0V
const float MAX_TEMP_SIMULATED = 100.0; // Corresponde a 100% PWM / 3.3V

// A resolução do ADC do ESP32 é 12 bits por padrão (0-4095)
const int ADC_MAX_VALUE = 4095; // Para 12 bits de resolução

volatile float simulatedTemperature = MIN_TEMP_SIMULATED; // Temperatura inicial simulada

// Callback: master requisitou dados
void onRequest() {
  float temp_copy = simulatedTemperature; 
  Serial.printf("RequestEvent: Mestre requisitou dados. Enviando temperatura: %.2f C\n", temp_copy);
  byte data[4];
  memcpy(data, &temp_copy, 4); 
  Wire.write(data, 4); 
}

// Callback: master enviou dados (debug)
void onReceive(int howMany) {
  Serial.printf("ReceiveEvent: Mestre enviou %d bytes. ", howMany);
  while (Wire.available()) {
    byte c = Wire.read(); 
    Serial.print((char)c);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(200); 
  Serial.println("\nESP32 I2C Slave Simulator - Iniciando...");

  // Configura a resolução do ADC
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
}

void loop() {
  // Lê o valor do ADC (representa a tensão do PWM filtrada)
  int adc_value = analogRead(ADC_INPUT_PIN);

  // Mapeia o valor do ADC para a faixa de temperatura simulada (25°C a 100°C)
  // O ADC vai de 0 a ADC_MAX_VALUE (4095).
  // A função map do Arduino é para inteiros, vai fazer a proporção manualmente para float
  simulatedTemperature = MIN_TEMP_SIMULATED + 
                         (float)adc_value * (MAX_TEMP_SIMULATED - MIN_TEMP_SIMULATED) / ADC_MAX_VALUE;

  // Limitar a temperatura simulada aos limites (caso o sinal analógico não seja perfeito)
  if (simulatedTemperature < MIN_TEMP_SIMULATED) {
    simulatedTemperature = MIN_TEMP_SIMULATED;
  }
  if (simulatedTemperature > MAX_TEMP_SIMULATED) {
    simulatedTemperature = MAX_TEMP_SIMULATED;
  }

  // Imprime a temperatura simulada -- Debug
  Serial.printf("Slave: ADC = %d, Temperatura Simulada = %.2f C\n", adc_value, simulatedTemperature);
  
  delay(100); // Pequeno delay para o loop
}