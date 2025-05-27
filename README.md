# Gym Access: Painel de Controle Interativo com Acesso Concorrente

Este projeto implementa um sistema de controle de acesso para uma academia utilizando a placa **BitDogLab com RP2040**, com gerenciamento de tarefas via **FreeRTOS** e uso de semáforos e mutexes para controle concorrente.

---

## Objetivo
O objetivo do projeto é implementar um sistema que permita o controle de alunos em uma academia, permitindo adicioná-los ou removê-los, desde que a quantidade atual esteja entre 0 e o máximo de alunos permitido no ambiente.

---

## Funcionalidades

- **Controle de entrada e saída de usuários**
- **Reset do sistema por interrupção**
- **Display OLED com informações em tempo real**
- **LED RGB indicando estado de ocupação**
- **Buzzer sonoro para alertas**
- **Gerenciamento de tarefas com FreeRTOS**
- **Uso de semáforo de contagem como contador de usuários**

---

## Componentes Utilizados

| Componente     | Função no Sistema                   |
|----------------|-------------------------------------|
| Botão A (GPIO 5) | Entrada de usuário (`xSemaphoreGive`) |
| Botão B (GPIO 6) | Saída de usuário (`xSemaphoreTake`)  |
| Botão do Joystick (GPIO 22) | Reset total (via interrupção) |
| LED RGB (GPIOs 11, 12, 13) | Indica estado da ocupação |
| Buzzer PWM (GPIO 21) | Alertas sonoros |
| Display OLED I2C | Exibe o título e a quantidade de usuários |
| Semáforo de contagem | Representa diretamente a quantidade de usuários ativos |
| Mutexes | Garantem acesso exclusivo ao display e LED |

---

## Cores do LED RGB

- **Azul**: Academia vazia
- **Verde**: Ocupação parcial
- **Amarelo**: Apenas 1 vaga restante
- **Vermelho**: Capacidade máxima atingida

---

## Estrutura do Código

- `vTaskEntrada`: Registra a entrada de usuários, se houver vagas.
- `vTaskSaida`: Registra a saída de usuários.
- `vTaskReset`: Reseta o sistema quando o botão do joystick é pressionado.
- `atualizar_display()`: Atualiza o display OLED.
- `atualizar_led()`: Atualiza o LED RGB conforme o número de usuários.
- `uxSemaphoreGetCount()`: Função utilizada para obter o número atual de usuários ativos.
