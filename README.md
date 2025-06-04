# Controlador de Temperatura para Produção de Cerveja Artesanal

Este projeto tem como objetivo o desenvolvimento de um sistema embarcado para auxiliar no processo de fabricação de cerveja artesanal, controlando a temperatura e o mixer (agitação) do mosto, com base em curvas programáveis de temperatura vs tempo.

## Objetivos

- Controlar a temperatura em tempo real utilizando controle PID ou ON/OFF com histerese.
- Ativar o mixer com base em condições específicas.
- Permitir a simulação da lógica no PC, sem hardware físico.
- Garantir segurança com detecção de falhas nos sensores.
- Prover interface com o usuário via PC e LCD.

---

## Tecnologias e Ferramentas

- ESP32
- C/C++
- Yakindu / itemis CREATE (opcional para máquina de estados)
- LCD 4x16
- Sensor de Temperatura (I2C)
- Comunicação UART com o PC
- Doxygen (documentação)
- Git/GitHub (versionamento)

---

## Arquitetura do Sistema

```plaintext
Sensor de Temperatura (I2C)
        ↓
Microcontrolador (PID + UI)
├── LCD 4x16
├── Resistência (GPIO/PWM)
├── Mixer (GPIO/PWM)
├── Botões
└── ESP32 UART ⇄ PC (Interface)
```

---

## Requisitos Funcionais

- Curva de temperatura padrão (ex: 67°C → 78°C → 100°C).
- Inserção de novas curvas via UART.
- Controle PID ou ON/OFF com histerese.
- Mixer é ativado quando a diferença entre dois sensores ≥ 1°C.
- Alerta e desativação em caso de falha no sensor.
- Máquina de estados via Yakindu ou C++ orientado a objetos.
- Simulação possível em ambiente de PC.

---

### Entregas

##### Entrega 1:
- Levantamento de requisitos
- Diagrama do sistema
- Curva padrão
- Modelagem da máquina de estados

##### Entrega 2:
- Drivers de sensores/atuadores
- Controle PID/ON-OFF funcional
- Comunicação com PC via UART

##### Entrega 3:
- Integração total
- Funcionalidade extra (ex: alarme, logs)
- Detecção de falhas
- Documentação (Doxygen)
- Demonstração

---

### Funcionalidade Extra (a definir)
##### Exemplos possíveis:
- Alarme sonoro
- Visualização gráfica da curva
- Modo de calibração
- Registro de logs

---

### Organização do Projeto
```plaintext
src/
├── main.cpp
├── controller/
│   ├── pid_controller.cpp
│   └── onoff_controller.cpp
├── drivers/
│   ├── sensor_temp.cpp
│   ├── heater.cpp
│   └── mixer.cpp
└── interface/
    ├── pc_uart.cpp
    └── lcd.cpp

docs/
└── README.md

yakindu/
└── statechart.sct (se aplicável)

Doxyfile (para geração da documentação)
```

---

### Ideias iniciais:
```plaintext
[Inicialização]
      ↓
    Idle ────────┐
      ↓          │
  Aquecendo      │
      ↓          │
Estabilizando    │
      ↓          │
 MixerLigado     │
      ↓          │
PróximoDegrau    |
      ↓          │
 Finalizado      │
      ↓          │
    (FIM) ◄──────┘
```

### Eventos e transições
| Evento | Transição |
| :--------: | :-------: |
| start | Idle → Aquecendo |
| tempOk | Aquecendo → Estabilizando |
| deltaT > 1°C | Estabilizando → MixerLigado |
| degrauCompleto | MixerLigado → PróximoDegrau |
| fimDaCurva | PróximoDegrau → Finalizado |
| erroSensor / falha | Qualquer estado → Erro |

### Changelog

- release/1.0.0: criar repositório com requisitos e o README
- release/2.0.0: código funcional de um blink com ESP32 e LED
- release/2.1.0: ajuste dos estados e códigos para simulação inicial com ESP32 e interação com estados e UART  
	
	
	
