/*
Você deverá implementar um painel de controle interativo, simulando o controle de acesso de
usuários a um determinado espaço (ex: laboratório, biblioteca, refeitório). O sistema será baseado na
placa BitDogLab com o RP2040 e utilizará:
• Semáforo de contagem para controlar o número de usuários simultâneos.
• Semáforo binário com interrupção para resetar o sistema.
• Mutex para proteger o acesso ao display OLED.
O sistema deve oferecer:
• Feedback visual através do LED RGB, indicando a ocupação.
• Sinalização sonora com o buzzer.
• Exibição de mensagens e contagem no display OLED.

O sistema simulado será o controle de acesso de uma academia
 */

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "lib/ssd1306.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "pico/bootrom.h"
#include "stdio.h"
#include "hardware/pwm.h"


#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C

#define BOTAO_A 5 // Incrementa contagem
#define BOTAO_B 6 // Decrementa contagem
#define SW_PIN 22 // Botão de reset
#define MAX_ALUNOS 10 // Máximo de usuários permitidos
#define BUZZER_A 21 // Buzzer
#define LED_PIN_GREEN 11
#define LED_PIN_BLUE 12
#define LED_PIN_RED 13

ssd1306_t ssd;
SemaphoreHandle_t xSemaforoAlunos;
SemaphoreHandle_t xSemaforoReset;
SemaphoreHandle_t xMutexDisplay;
int usuariosAtivos = 0;

// ISR do botão SW_PIN para resetar o sistema
void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == SW_PIN) {
        xSemaphoreGiveFromISR(xSemaforoReset, NULL);
    }
}


void atualizar_display() {
    char buffer[20];

    ssd1306_fill(&ssd, 0);
    sprintf(buffer, "alunos: %d", usuariosAtivos);
    ssd1306_draw_string(&ssd, "Quantidade", 5, 10);
    ssd1306_draw_string(&ssd, "de", 30, 20);
    ssd1306_draw_string(&ssd, buffer, 5, 35);
    ssd1306_send_data(&ssd);
    xSemaphoreGive(xMutexDisplay);
}


void atualizar_led() {
    if (usuariosAtivos == 0) {
        gpio_put(LED_PIN_RED, 0);
        gpio_put(LED_PIN_GREEN, 0);
        gpio_put(LED_PIN_BLUE, 1); // Azul
    } else if (usuariosAtivos < MAX_ALUNOS - 1) {
        gpio_put(LED_PIN_RED, 0);
        gpio_put(LED_PIN_GREEN, 1);
        gpio_put(LED_PIN_BLUE, 0); // Verde
    } else if (usuariosAtivos == MAX_ALUNOS - 1) {
        gpio_put(LED_PIN_RED, 1);
        gpio_put(LED_PIN_GREEN, 1);
        gpio_put(LED_PIN_BLUE, 0); // Amarelo
    } else {
        gpio_put(LED_PIN_RED, 1);
        gpio_put(LED_PIN_GREEN, 0);
        gpio_put(LED_PIN_BLUE, 0); // Vermelho
    }
}

// TAREFAS


// Aumenta o número de usuários ativos. Caso o limite seja atingido, exibe aviso e emite um beep.
void vTaskEntrada(void *params) {
    while (1) {
        if (!gpio_get(BOTAO_A)) {
            if (xSemaphoreTake(xSemaforoAlunos, 0) == pdTRUE) {
                if(xSemaphoreTake(xMutexDisplay, portMAX_DELAY) == pdTRUE) {
                    usuariosAtivos++;
                    atualizar_display();
                    atualizar_led();
                    xSemaphoreGive(xMutexDisplay);
                }
            } else {
                // Capacidade cheia
                printf("Capacidade máxima atingida!\n");

                // Beep
                pwm_set_gpio_level(BUZZER_A, 100);
                vTaskDelay(pdMS_TO_TICKS(100));
                pwm_set_gpio_level(BUZZER_A, 0);
            }
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


// Reduz o número de usuários ativos (se houver).
void vTaskSaida(void *params) {
    while (1) {
        if (!gpio_get(BOTAO_B)) {
            if (usuariosAtivos > 0) {
                xSemaphoreGive(xSemaforoAlunos);
                if(xSemaphoreTake(xMutexDisplay, portMAX_DELAY) == pdTRUE) {
                    usuariosAtivos--;
                    atualizar_display();
                    atualizar_led();
                    xSemaphoreGive(xMutexDisplay);
                }
            }
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Zera a contagem de usuários e gera beep duplo
void vTaskReset(void *params) {
    while (1) {
        if (xSemaphoreTake(xSemaforoReset, portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(xMutexDisplay, portMAX_DELAY) == pdTRUE) {
                usuariosAtivos = 0;
                while (usuariosAtivos > 0) {
                    usuariosAtivos--;
                    xSemaphoreGive(xSemaforoAlunos);
                }

                atualizar_display();
                atualizar_led();
                xSemaphoreGive(xMutexDisplay);
            }
            
            // Beep duplo
            pwm_set_gpio_level(BUZZER_A, 100);
            vTaskDelay(pdMS_TO_TICKS(100));

            pwm_set_gpio_level(BUZZER_A, 0);
            vTaskDelay(pdMS_TO_TICKS(100));

            pwm_set_gpio_level(BUZZER_A, 100);
            vTaskDelay(pdMS_TO_TICKS(100));

            pwm_set_gpio_level(BUZZER_A, 0);
        }
    }
}

// ISR para BOOTSEL e botão de evento
void gpio_irq_handler(uint gpio, uint32_t events)
{
    if (gpio == SW_PIN)
    {
        static uint32_t ultimo_acionamento = 0;
        uint32_t agora = time_us_32();
        // 200 ms = 200000 us
        if (agora - ultimo_acionamento >= 200000) {
            ultimo_acionamento  = agora;
            gpio_callback(gpio, events);
        }
        // Se for dentro do tempo de debounce, ignora o evento
    }
}

void setupPerifericos()
{
    // Configura o LED RGB
    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);
    gpio_put(LED_PIN_GREEN, 0);

    gpio_init(LED_PIN_BLUE);
    gpio_set_dir(LED_PIN_BLUE, GPIO_OUT);
    gpio_put(LED_PIN_BLUE, 0);

    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);
    gpio_put(LED_PIN_RED, 0);

    // Configura o buzzer
    gpio_set_function(BUZZER_A, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_A); // Obtém o slice correspondente
    pwm_set_clkdiv(slice_num, 125); // Define o divisor de clock
    pwm_set_wrap(slice_num, 1000);  // Define o valor máximo do PWM
    pwm_set_enabled(slice_num, true);
    
    // Configura os botões
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);

    gpio_init(SW_PIN);
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_pull_up(SW_PIN);
}

int main()
{
    stdio_init_all();

    // Inicialização do display
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    // Configura os periféricos
    setupPerifericos();
    atualizar_led();
    // Configura interrupção do botão de reset
    gpio_set_irq_enabled_with_callback(SW_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Cria semáforo de contagem e mutex
    xSemaforoAlunos = xSemaphoreCreateCounting(MAX_ALUNOS, MAX_ALUNOS);
    xSemaforoReset = xSemaphoreCreateBinary();
    xMutexDisplay = xSemaphoreCreateMutex();

    // Cria tarefa
    xTaskCreate(vTaskEntrada, "TaskEntrada", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskSaida, "TaskSaida", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskReset, "TaskReset", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);

    vTaskStartScheduler();
    panic_unsupported();
}
