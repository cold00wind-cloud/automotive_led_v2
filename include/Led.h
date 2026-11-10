#pragma once
#include "stm32f4xx_hal.h" 
#include "stdbool.h"
    
typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    uint32_t mode;
    GPIO_PinState activeState;
    uint32_t lastToggle;
    bool isOn;

} Led;

void led_init(Led *led);
void led_set(Led *led, bool set);
void led_blink(Led *led, uint32_t period_ms);

