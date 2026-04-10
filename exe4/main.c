#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include "pico/stdlib.h"
#include <stdio.h>

const int BTN_PIN_R = 28;
const int BTN_PIN_G = 26;
const int LED_PIN_R = 4;
const int LED_PIN_G = 6;


QueueHandle_t xQueueButId;
QueueHandle_t xQueueBtn2;

QueueHandle_t xQueueDataR;
QueueHandle_t xQueueDataG;

SemaphoreHandle_t xSemaphore_r;
SemaphoreHandle_t xSemaphore_g;

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { // fall edge
        if (gpio == BTN_PIN_R){
            xSemaphoreGiveFromISR(xSemaphore_r, NULL);
            xQueueSendFromISR(xQueueDataR, &gpio, NULL);
        } else {
            xSemaphoreGiveFromISR(xSemaphore_g, NULL);
            xQueueSendFromISR(xQueueDataG, &gpio, NULL);
        }
    }
}


void led_1_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);
    int delay = 0;
    while (true) {
        
        xQueueReceive(xQueueButId, &delay, 0);
        if (delay > 0) {
            gpio_put(LED_PIN_R, 1);
            vTaskDelay(pdMS_TO_TICKS(delay));
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(delay));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}


void led_2_task(void *p) {
    gpio_init(LED_PIN_G);
    gpio_set_dir(LED_PIN_G, GPIO_OUT);
    int delay = 0;
    while (true) {
        xQueueReceive(xQueueBtn2, &delay, 0);
        if (delay > 0) {
            gpio_put(LED_PIN_G, 1);
            vTaskDelay(pdMS_TO_TICKS(delay));
            gpio_put(LED_PIN_G, 0);
            vTaskDelay(pdMS_TO_TICKS(delay));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}


void btn_1_task(void *p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);
    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_callback);

    int delay = 0;
    int pino_recebido; 
    while (true) {
        if (xSemaphoreTake(xSemaphore_r, portMAX_DELAY) == pdTRUE) {
            if (xQueueReceive(xQueueDataR, &pino_recebido, 0)) {
                delay = (delay >= 1000) ? 100 : delay + 100;
                xQueueSend(xQueueButId, &delay, 0);
            }
        }
    }
}


void btn_2_task(void *p) {
    gpio_init(BTN_PIN_G);
    gpio_set_dir(BTN_PIN_G, GPIO_IN);
    gpio_pull_up(BTN_PIN_G);
    gpio_set_irq_enabled(BTN_PIN_G, GPIO_IRQ_EDGE_FALL, true);

    int delay = 0;
    int pino_recebido;
    while (true) {
        if (xSemaphoreTake(xSemaphore_g, portMAX_DELAY) == pdTRUE) {
            if (xQueueReceive(xQueueDataG, &pino_recebido, 0)) {
                delay = (delay >= 1000) ? 100 : delay + 100;
                xQueueSend(xQueueBtn2, &delay, 0);
            }
        }
    }
}

int main() {
    stdio_init_all();
    
    xQueueDataR = xQueueCreate(32, sizeof(int));
    xQueueDataG = xQueueCreate(32, sizeof(int));
    
    
    xQueueButId = xQueueCreate(32, sizeof(int));
    xQueueBtn2 = xQueueCreate(32, sizeof(int));

    xSemaphore_r = xSemaphoreCreateBinary();
    xSemaphore_g = xSemaphoreCreateBinary();

    xTaskCreate(led_1_task, "LED_R", 256, NULL, 1, NULL);
    xTaskCreate(led_2_task, "LED_G", 256, NULL, 1, NULL);
    xTaskCreate(btn_1_task, "BTN_R", 256, NULL, 1, NULL);
    xTaskCreate(btn_2_task, "BTN_G", 256, NULL, 1, NULL);

    vTaskStartScheduler();
    while (true);
}