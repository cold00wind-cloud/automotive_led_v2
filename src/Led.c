#include "Led.h"

void led_init(Led *led)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = led->pin;
    gpio.Mode = led->mode;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(led->port, &gpio);
    HAL_GPIO_WritePin(led->port, led->pin, led->activeState == GPIO_PIN_SET ? GPIO_PIN_RESET : GPIO_PIN_SET);
    led->lastToggle = 0;
    led->isOn = false;
}

void led_set(Led *led, bool on){
    HAL_GPIO_WritePin(
        led->port, 
        led->pin, 
        on ? led->activeState : !led->activeState);
    led->isOn = on;
}

void led_blink(Led *led, uint32_t period_ms )
{
    uint32_t now = HAL_GetTick();
    if (now - led->lastToggle > period_ms )
    {
        led->lastToggle = now;
        HAL_GPIO_WritePin(
            led->port, led->pin,
            !HAL_GPIO_ReadPin(led->port, led->pin));
        led->isOn = !led->isOn;
    }
}