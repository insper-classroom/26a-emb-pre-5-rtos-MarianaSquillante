#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>

const int BTN_PIN_R = 28;
const int BTN_PIN_G = 26;

const int LED_PIN_R = 5;
const int LED_PIN_G = 10;

QueueHandle_t xQueueLedR;
QueueHandle_t xQueueLedG;
QueueHandle_t xQueueBtn;

void btn_callback(uint gpio, uint32_t events) {
  int btn = 0;
  if(events == GPIO_IRQ_EDGE_FALL){
    if(gpio == BTN_PIN_G){
      btn = 2;
    }
    if(gpio == BTN_PIN_R){
      btn = 1;
    }
  }
  if(btn != 0){
    xQueueSendFromISR(xQueueBtn, &btn, 0);
  }
}

void led_1_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);
    gpio_put(LED_PIN_R, 0);

    int delay = 0;
    while (true) {
        if (xQueueReceive(xQueueLedR, &delay, 0)) {
            //printf("%d\n", delay);
        }

        if (delay > 0) {
            gpio_put(LED_PIN_R, 1);
            vTaskDelay(pdMS_TO_TICKS(delay));
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(delay));
        }
    }
}

void led_2_task(void *p) {
    gpio_init(LED_PIN_G);
    gpio_set_dir(LED_PIN_G, GPIO_OUT);
    gpio_put(LED_PIN_G, 0);

    int delay = 0;
    while (true) {
        if (xQueueReceive(xQueueLedG, &delay, 0)) {
            //printf("%d\n", delay);
        }

        if (delay > 0) {
            gpio_put(LED_PIN_G, 1);
            vTaskDelay(pdMS_TO_TICKS(delay));
            gpio_put(LED_PIN_G, 0);
            vTaskDelay(pdMS_TO_TICKS(delay));
        }
    }
}

void btn_task(void *p) {
    int delay_r = 0;
    int delay_g = 0;
    int btn;
    while (true) {
        if (xQueueReceive( xQueueBtn, &btn, pdMS_TO_TICKS(100))) {

            if(btn == 2){
                if (delay_g < 1000) {
                    delay_g += 100;
                } else {
                    delay_g = 100;
                }
                //printf("delay btn %d \n", delay_g);
                xQueueSend(xQueueLedG, &delay_g, 0);
            }

            if(btn == 1){
                if (delay_r < 1000) {
                    delay_r += 100;
                } else {
                    delay_r = 100;
                }
                //printf("delay btn %d \n", delay_r);
                xQueueSend(xQueueLedR, &delay_r, 0);
            }
            
        }
    }
}

int main() {
    stdio_init_all();

    gpio_init(BTN_PIN_G);
    gpio_set_dir(BTN_PIN_G, GPIO_IN);
    gpio_pull_up(BTN_PIN_G);
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);
    //printf("Start RTOS \n");

    xQueueBtn = xQueueCreate(32, sizeof(int));
    xQueueLedG = xQueueCreate(32, sizeof(int));
    xQueueLedR = xQueueCreate(32, sizeof(int));

    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_G, GPIO_IRQ_EDGE_FALL, true);

    xTaskCreate(led_1_task, "LED_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(led_2_task, "LED2_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(btn_task, "BTN_Task 1", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
