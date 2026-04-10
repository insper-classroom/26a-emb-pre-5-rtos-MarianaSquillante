#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include "pico/stdlib.h"
#include <stdio.h>

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;
const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

// Recursos do RTOS
QueueHandle_t xQueueBtn;
SemaphoreHandle_t xSemaphoreLedR;
SemaphoreHandle_t xSemaphoreLedY;

// Callback de interrupção
void btn_callback(uint gpio, uint32_t events) {
    int btn_id = 0;
    if (events == GPIO_IRQ_EDGE_FALL) {
        if (gpio == BTN_PIN_R) {
            btn_id = 1; 
            xQueueSendFromISR(xQueueBtn, &btn_id, NULL);
        } else if (gpio == BTN_PIN_Y) {
            btn_id = 2; 
            xQueueSendFromISR(xQueueBtn, &btn_id, NULL);
        }
    }
}

void btn_task(void *p) {
    int btn_recebido;
    while (true) {
        
        if (xQueueReceive(xQueueBtn, &btn_recebido, portMAX_DELAY)) {
            if (btn_recebido == 1) {
                // Inverte o estado do LED Vermelho 
                xSemaphoreGive(xSemaphoreLedR);
            } else if (btn_recebido == 2) {
                // Inverte o estado do LED Amarelo 
                xSemaphoreGive(xSemaphoreLedY);
            }
        }
    }
}


void led_r_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);
    int status = 0; // 0 = parado, 1 = piscando

    while (true) {
        // Tenta pegar o semáforo para mudar o estado (não bloqueante)
        if (xSemaphoreTake(xSemaphoreLedR, 0) == pdTRUE) {
            status = !status;
            if (!status) gpio_put(LED_PIN_R, 0); // Garante que apaga ao parar
        }

        if (status) {
            gpio_put(LED_PIN_R, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10)); // Evita starvation
        }
    }
}


void led_y_task(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);
    int status = 0;

    while (true) {
        if (xSemaphoreTake(xSemaphoreLedY, 0) == pdTRUE) {
            status = !status;
            if (!status) gpio_put(LED_PIN_Y, 0);
        }

        if (status) {
            gpio_put(LED_PIN_Y, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

int main() {
    stdio_init_all();

  
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);

    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);

    xQueueBtn = xQueueCreate(10, sizeof(int));
    xSemaphoreLedR = xSemaphoreCreateBinary();
    xSemaphoreLedY = xSemaphoreCreateBinary();

    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    xTaskCreate(led_r_task, "LED_R", 256, NULL, 1, NULL);
    xTaskCreate(led_y_task, "LED_Y", 256, NULL, 1, NULL);
    xTaskCreate(btn_task, "BTN_Logic", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true);
}