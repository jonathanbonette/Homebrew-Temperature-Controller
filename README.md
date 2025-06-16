# 🍺 Controlador de Temperatura para Produção de Cerveja Artesanal

Um sistema embarcado robusto e flexível para monitorar e controlar a temperatura e a agitação do mosto durante o processo de fabricação de cerveja artesanal, utilizando curvas de temperatura programáveis.

### Visão Geral do Projeto
Este projeto visa desenvolver um controlador de processo inteligente para auxiliar cervejeiros artesanais. O sistema monitora e regula, em tempo real, a temperatura do mosto e aciona um mixer para garantir a homogeneidade, seguindo perfis de temperatura predefinidos ou customizáveis. A arquitetura modular e a capacidade de simulação em PC garantem robustez e facilidade de desenvolvimento.

### Objetivos Principais

* **Controle Preciso de Temperatura:** Manter a temperatura do mosto dentro de margens específicas ($\pm 0.5^\circ C$) utilizando algoritmos PID e/ou ON/OFF com histerese.
* **Homogeneização Otimizada:** Acionar automaticamente o mixer com base na diferença de temperatura entre múltiplos pontos do mosto.
* **Flexibilidade de Processo:** Permitir a definição e o *upload* de curvas de temperatura e "receitas" de brassagem.
* **Segurança e Confiabilidade:** Implementar detecção de falhas (sensores, comunicação) com alertas visuais e sonoros e mecanismos de resiliência.
* **Interface Amigável:** Prover *feedback* contínuo e interação com o usuário via *display* OLED/LCD e comunicação serial com PC.
* **Portabilidade e Testabilidade:** Garantir que a lógica de controle possa ser simulada em PC para testes sem *hardware* e seja portável para diferentes plataformas embarcadas (ESP32).


---

### Tecnologias e Ferramentas Utilizadas

* **Microcontrolador:** ESP32 (Plataforma.io com FreeRTOS)
* **Linguagem de Programação:** C++ (com princípios de Orientação a Objetos)
* **Modelagem de Estados:** Yakindu / itemis CREATE (para máquinas de estados)
* **Interface Humano-Máquina (IHM):**
    * Display OLED/LCD (via I2C)
    * Teclado Matricial 4x4
    * Comunicação Serial (UART) para interface com PC
    * **// TODO: colocar o restante conforme o projeto for desenvolvendo**
* **Sensores:** Sensores de Temperatura (I2C)
* **Atuadores:** Resistências de Aquecimento (via GPIO/PWM), Mixer (via GPIO/PWM)
* **Versionamento:** Git & GitHub
* **Documentação:** Doxygen
* **Testes:** Unity / Google Test (**Em definição por ser um RNF**)

---

### Arquitetura do Sistema
A arquitetura do sistema é centrada no Microcontrolador ESP32, que orquestra as leituras dos sensores, a lógica de controle e a atuação nos periféricos, além de gerenciar a interação com o usuário e o PC.

```plaintext
+---------------------+      +-------------------+      +-----------------+
| Sensores de Temp.   |<-----| Sinal Analógico   |<-----|                   |
| (Multiplos, I2C)    |      |                   |      | Microcontrolador  |
+---------------------+      |                   |      | (ESP32, FreeRTOS) |
                             |                   |      | (PID + Statechart)|
+---------------------+      |                   |      |                   |
| Teclado 4x4         |<-----| Entradas Digitais |<-----|                   |
+---------------------+      |                   |      |                   |
                             |                   |      |                   |
+---------------------+      +-------------------+      +-----------------+
| Display OLED/LCD    |<-----| Saída I2C         |<-----|                   |
+---------------------+      |                   |      |                   |
                             |                   |      |                   |
+---------------------+      +-------------------+      |                   |
| Resistências        |<-----| Sinal de PWM      |<-----|                   |
| (Heaters)           |      |                   |      |                   |
+---------------------+      |                   |      |                   |
                             |                   |      |                   |
+---------------------+      +-------------------+      |                   |
| Mixer               |<-----| Sinal de PWM      |<-----|                   |
+---------------------+      |                   |      |                   |
                             |                   |      |                   |
                             +-------------------+      +-----------------+
                                       ^                      |
                                       |                      | TX/RX (UART)
                                       |                      |
                                       |                      v
                                 +----------------+
                                 | PC (Interface) |
                                 +----------------+
```

---

### Requisitos do Projeto
Os requisitos foram categorizados em Funcionais (o que o sistema deve fazer) e Não Funcionais (como o sistema deve funcionar). O progresso de cada requisito é visualizado através de uma barra de status.

| ID | Tipo | Requisito | Prioridade | Status | Progresso | Entrega Relacionada | Observações |
| :---- | :---- | :---- | :---- | :---- | :---- | :---- | :---- |
| RF01  | Funcional   | O sistema deve aplicar algoritmos de controle (PID e/ou ON/OFF com histerese) para manter a temperatura do mosto dentro de uma margem de $\pm 0.5^\circ C$ do *setpoint* definido pela curva de brassagem, acionando as resistências de aquecimento. | Alta | Em análise | [🟦🟦🟦🟦🟦] 0% | Entrega 2 | Especifica a precisão do controle. O PID é esperado para ser robusto. |
| RF02  | Funcional | O sistema deve carregar e executar uma curva de temperatura padrão pré-configurada (ex: 67°C por X min, 78°C por Y min, 100°C por Z min) ao iniciar a brassagem. | Alta          | Em análise    | [🟦🟦🟦🟦🟦] 0% | Entrega 1 | Deixa claro que a curva padrão é a *default* e como ela é iniciada. |
| RF03 | Funcional | O sistema deve permitir o *upload* de curvas de temperatura customizadas via interface serial (UART), ou via comandos em tela (KEYPAD 4X4). Cada curva deve conter múltiplos *steps* definidos por temperatura e duração, e o sistema deve validar a integridade dos dados recebidos. | Alta | Pendente | [🟥🟥🟥🟥🟥] 0% | Entrega 2 | Detalha a forma de *upload* e a necessidade de validação. |
| RF04  | Funcional | O sistema deve ativar automaticamente o *mixer* quando a diferença de temperatura entre quaisquer dois sensores exceder $1^\circ C$ (Delta T), com um retardo de ativação configurável, para garantir a homogeneização do mosto. | Alta | Em análise | [🟦🟦🟦🟦🟦] 0% | Entrega 2 | Necessário 2 sensores (TEMPERATURA) em dois extremos da bacia e lógica de comparação, da mesma forma um dispositivo com motor de rotação para o MIXER. |
| RF05  | Funcional | O sistema deve exibir continuamente no *display* OLED (ou LCD) a temperatura atual do mosto (obtida pelo sensor principal), o *setpoint* da etapa atual da curva, o tempo restante para a etapa atual e o *status* operacional do *mixer* e resistências. | Alta          | Pendente | [🟥🟥🟥🟥🟥] 0% | Entrega 2 | Especifica quais informações devem ser mostradas e de qual sensor. Detalha o "estado atual". |
| RF06  | Funcional   | O sistema deve emitir alertas visuais (LEDs) ou sonoros (BUZZER) para notificar o usuário sobre erros críticos de operação (ex: falha de sensor, sobreaquecimento, erro de comunicação), indicando o tipo de falha. | Alta | Pendente | [🟥🟥🟥🟥🟥] 0% | Entrega 3 | Especifica os tipos de alerta e a informação a ser passada (tipo de falha). |
| RF07  | Funcional | O sistema deve permitir ao usuário selecionar o modo de controle (PID ou ON/OFF) antes ou durante o início de uma nova brassagem, através da interface de usuário (teclado ou serial). | Alta          | Pendente | [🟥🟥🟥🟥🟥] 0% | Entrega 2 | Define quando a seleção pode ocorrer e por qual interface. |
| RF08  | Funcional   | O sistema deve implementar um "Modo de Calibração" para os sensores de temperatura, permitindo ao usuário ajustar *offsets* ou fatores de calibração para leituras mais precisas, com base em temperaturas de referência conhecidas. | Baixa | Pendente      | [🟥🟥🟥🟥🟥] 0% | Entrega 3 | Funcionalidade adicional interessante. |
| RF09  | Funcional   | O sistema deve permitir ao usuário visualizar e modificar parâmetros específicos da curva de brassagem em tempo real (ex: *setpoint* de temperatura, duração da etapa) através da comunicação UART, sem interromper o processo atual. | Baixa         | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 2 | Detalha a modificação de curvas durante a operação, um requisito mais avançado que "modificação de curvas". |
| RF10  | Funcional   | O sistema deve registrar em memória não volátil (*Flash* ou EEPROM) os parâmetros de cada brassagem concluída (curva utilizada, temperaturas máximas/mínimas atingidas, duração total) e permitir a consulta desses *logs* via UART. | Média         | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 3 | Funcionalidade adicional de histórico, útil para otimização de receitas. |
| RF11  | Funcional   | O sistema deve suportar a criação de "receitas", que são sequências pré-definidas de curvas de brassagem (ex: *Mash*, *Boil*, *Fermentação*), permitindo ao usuário selecionar e executar uma receita completa. | Alta | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 3 | Eleva o projeto de controle de uma única curva para o gerenciamento de múltiplos estágios da produção de cerveja. |
| RF12  | Funcional   | O sistema deve fornecer *feedback* visual (ex: ícones no *display* ou LEDs de *status*) sobre o estado atual dos atuadores (resistências ligadas/desligadas, *mixer* ativo/inativo), mesmo quando não houver erro crítico, para facilitar o monitoramento do processo. | Média | Pendente | [🟥🟥🟥🟥🟥] 0%      | Entrega 2           | Melhora a usabilidade e a visualização do estado do sistema em tempo real. |
| RNF01 | Não Funcional | A arquitetura de *software* deve ser portável, permitindo que o sistema seja executado tanto em uma plataforma embarcada (ESP32) quanto em um ambiente de simulação no PC, utilizando abstração de *hardware* (HAL) para facilitar a troca de implementações de periféricos. | Alta | Em análise    | [🟦🟦🟦🟦🟦] 0%      | Entrega 3           | Enfatiza a portabilidade e o uso de HAL para atingir esse objetivo. |
| RNF02 | Não Funcional | O código-fonte deve ser implementado em C++ seguindo princípios de Orientação a Objetos (OO), com uso extensivo de polimorfismo e métodos virtuais, para garantir modularidade, reusabilidade e extensibilidade do sistema. | Alta | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Todas | Aprofunda em "boas práticas" especificando OO, polimorfismo e métodos virtuais, o que é fundamental para a simulação no PC. |
| RNF03 | Não Funcional | O projeto deve ser versionado no GitHub com um histórico de *commits* claro e descritivo, refletindo o progresso incremental e as mudanças significativas em cada funcionalidade implementada. | Alta | Em andamento  | [🟩🟩🟥🟥🟥] 40%      | Todas | Adiciona "histórico de *commits* claro e descritivo" para enfatizar a qualidade do versionamento. |
| RNF04 | Não Funcional | Toda a base de código deve ser documentada utilizando Doxygen, gerando uma documentação técnica completa das funções, classes, variáveis e módulos, facilitando a compreensão e manutenção por outros desenvolvedores. | Alta | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 3 | Especifica "toda a base de código" e o benefício da documentação. |
| RNF05 | Não Funcional | O sistema deve incorporar proteção elétrica robusta contra curtos-circuitos, sobreaquecimento e surtos de tensão nos circuitos de controle das resistências e do *mixer*, garantindo a segurança do equipamento e do usuário. | Baixa         | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 3           | Projeto do circuito com fusíveis, termistores e testes elétricos necessários. |
| RNF07 | Não Funcional | Deve ser fornecido um manual de montagem detalhado, com diagramas de fiação, fotos ilustrativas e lista de materiais (BOM), para permitir que um usuário replique o *hardware* do sistema. | Média         | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 3           | Produzir manual detalhado com fotos e diagramas de montagem. |
| RNF08 | Não Funcional | O sistema deve gerar *logs* de eventos detalhados (ex: mudanças de *setpoint*, ativação/desativação de atuadores, erros de sensor) via UART, que possam ser facilmente consumidos e analisados por uma aplicação no PC para depuração e monitoramento. | Alta          | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 3           | Implementar módulo dedicado de *logging* via UART ou armazenamento local. |
| RNF09 | Não Funcional | O *software* deve ser projetado com alta coesão e baixo acoplamento entre os módulos, utilizando interfaces bem definidas para facilitar futuras expansões e manutenções sem impacto em outras partes do sistema. | Média | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 2           | Estruturação de código usando orientação a objetos com interfaces claras. |
| RNF10 | Não Funcional | O sistema deve reagir a alterações nos sensores ou comandos de controle e atualizar o estado dos atuadores e do *display* em no máximo 500ms, para garantir uma experiência de usuário responsiva e controle em tempo real. | Baixa         | Em andamento  | [🟩🟥🟥🟥🟥] 20%      | Entrega 2           | Especifica um tempo de resposta (ex: 500ms) para tornar o requisito mensurável e mais exigente que 2s, dado o FreeRTOS. |
| RNF11 | Não Funcional | O *software* deve ser submetido a testes unitários automatizados para as camadas de lógica de controle e módulos críticos, utilizando um *framework* de teste (ex: Unity, Google Test) para garantir a robustez e correção do código. | Baixa         | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 3           | Desenvolver testes usando *frameworks* como Unity ou GoogleTest. |
| RNF12 | Não Funcional | O sistema deve permitir o ajuste manual da potência de saída para as resistências (ex: via PWM), permitindo ao usuário sobrescrever temporariamente o controle automático em situações específicas de *fine-tuning* ou emergência. | Baixa         | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 2           | Implementar entrada manual (GPIO ou serial) que consiga atuar em cima do controle automático. |
| RNF13 | Não Funcional | O sistema deve detectar e alertar sobre a perda de comunicação com qualquer sensor I²C (ex: por *timeout* ou falha de CRC), e tentar restabelecer a comunicação automaticamente antes de reportar uma falha crítica. | Baixa         | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 3           | Detalha a detecção da perda de comunicação e a tentativa de recuperação. |
| RNF14 | Não Funcional | O sistema deve ser capaz de calibrar automaticamente ou semi-automaticamente os sensores de temperatura durante o processo de brassagem, compensando variações devido à imersão ou tipo de sensor, utilizando pontos de calibração conhecidos. | Alta          | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 2           | Melhora e detalha a calibração, tornando-a mais avançada. |
| RNF15 | Não Funcional | A interface de usuário via teclado deve ser intuitiva e eficiente, permitindo a navegação pelos menus e a entrada de dados com um mínimo de passos, para uma boa experiência do usuário. | Média         | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 3           | Requisito de usabilidade importante, especialmente para interfaces embarcadas. |
| RNF16 | Não Funcional | O consumo de energia do sistema deve ser otimizado para operação de longo prazo, especialmente em modos de espera ou monitoramento, para minimizar o aquecimento desnecessário e potencializar o uso de fontes de energia. | Baixa         | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 2           | Requisito de eficiência e sustentabilidade. |
| RNF17 | Não Funcional | O código deve seguir um guia de estilo de codificação (ex: Google Style Guide, MISRA C/C++) para garantir consistência, legibilidade e manutenibilidade em todo o projeto. | Baixa         | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 3           | Requisito de qualidade de código mais formal. |
| RNF18 | Não Funcional | A interface de comunicação serial (UART) deve ser baseada em um protocolo bem definido (ex: ASCII com *checksum* ou JSON para comandos/dados), garantindo a robustez e a interoperabilidade com *softwares* externos (PC). | Alta          | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 2           | Extremamente importante para comunicação confiável e futura integração com GUIs de PC. Detalha a qualidade da comunicação. |
| RNF19 | Não Funcional | O sistema deve ter capacidade de *over-the-air (OTA) update* para o *firmware*, permitindo atualizações de *software* remotas sem a necessidade de conexão física via USB, facilitando a manutenção e a adição de novas funcionalidades pós-implantação. | Média         | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 3           | Um requisito de manutenção moderna para sistemas embarcados, crucial para a longevidade do produto. |
| RNF20 | Não Funcional | O *software* deve ser resiliente a falhas temporárias (ex: ruído elétrico, pequenas interrupções de comunicação), implementando mecanismos como *debouncing* para entradas digitais, *timeouts* com *retries* para comunicações e inicialização segura dos periféricos. | Alta          | Pendente      | [🟥🟥🟥🟥🟥] 0%      | Entrega 2           | Foca na robustez do *software* em ambientes ruidosos, essencial para um sistema de controle industrial leve. |

---

### Plano de Entregas
O projeto será desenvolvido em três entregas principais, com foco na progressão incremental das funcionalidades e na validação contínua.

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

### Organização do Projeto (Arrumar)
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

### Ideias iniciais (Arrumar)
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

### Eventos e transições (Arrumar)
| Evento | Transição |
| :--------: | :-------: |
| start | Idle → Aquecendo |
| tempOk | Aquecendo → Estabilizando |
| deltaT > 1°C | Estabilizando → MixerLigado |
| degrauCompleto | MixerLigado → PróximoDegrau |
| fimDaCurva | PróximoDegrau → Finalizado |
| erroSensor / falha | Qualquer estado → Erro |

### Changelog
Este changelog registra as principais versões e funcionalidades implementadas no projeto.

* **v0.1.0** - **Início do Projeto (Desenvolvimento Inicial)**
    * Criação do repositório no GitHub.
    * Definição e documentação dos requisitos iniciais e arquitetura.
    * Configuração do ambiente de desenvolvimento (PlatformIO).
* **v0.2.0** - **Prova de Conceito Básica**
    * Implementação de código funcional de "blink" com ESP32 e LED.
    * Demonstração de controle básico de GPIO.
* **v0.3.0** - **Integração Inicial com Statechart e UART**
    * Ajustes nos estados e código para simulação inicial com ESP32.
    * Integração com a máquina de estados (via itemis CREATE).
    * Comunicação básica via UART para *debug* e monitoramento.
* **v0.4.0** - **Integração com Display OLED**
    * Ajustes no código para a visualização dos estados com um Display OLED.
* **v0.5.0** - **Interface com Teclado Matricial**
    * Adição do teclado matricial 4x4 e implementação do *driver*.
    * Rotinas para leitura de teclas e integração com a Interface Humano-Máquina (IHM).
    * Navegação básica dos três primeiros menus.
