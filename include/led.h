#pragma once
/*
 * Світлодіод: увімкнути, вимкнути, блимати, спалахнути на певний час.
 *
 * led_blink() і led_pulse() лише запам'ятовують режим і час старту.
 * Саме перемикання робить led_update(), яку викликають з головного циклу.
 */
#include <stdbool.h>
#include "stm32f4xx_hal.h"

typedef enum
{
    LED_OFF,
    LED_ON,
    LED_BLINK, /* блимає, поки не скажуть інше */
    LED_PULSE, /* горить period мс, потім гасне сам */
} LedMode;

typedef struct
{
    /* Налаштування — заповнюєш сам при створенні */
    GPIO_TypeDef *port;
    uint16_t pin;
    GPIO_PinState active; /* рівень на піні, при якому LED світиться */

    /* Стан — веде сам LED */
    LedMode mode;
    bool lit;        /* чи світиться просто зараз */
    uint32_t since;  /* коли востаннє перемкнувся */
    uint32_t period; /* півперіод блимання або тривалість спалаху, мс */
} Led;

void led_init(Led *l); /* налаштовує пін, LED вимкнений */
void led_on(Led *l);
void led_off(Led *l);
void led_blink(Led *l, uint32_t half_period_ms); /* завжди починає з фази "горить" */
void led_pulse(Led *l, uint32_t ms);             /* повторний виклик починає відлік заново */

/* Викликати в кожному проході головного циклу. */
void led_update(Led *l);
