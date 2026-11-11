#pragma once
/*
 * Led (CL) — світлодіод зі своєю стейт-машиною.
 *
 * Старий led_blink() треба було викликати в циклі й перевіряти HAL_GetTick() —
 * це полінг. Тепер LED сам запускає свій програмний таймер і перемикається,
 * коли приходить SIG_TIMEOUT. Хто сказав "блимай", більше про це не думає.
 *
 *   OFF ──led_on──► ON
 *    ▲               │
 *    └──led_off──────┘
 *   будь-який ──led_pulse(ms)──► PULSE ──timeout──► OFF
 *   будь-який ──led_blink(ms)──► BLINK_ON ⇄ BLINK_OFF (кожні ms)
 */
#include <stdint.h>
#include "dl/gpio_pin.h"

typedef struct
{
    const Pin *pin;
    uint8_t timer; /* номер програмного таймера цього LED */
} LedConfig;

typedef enum
{
    LED_ST_OFF,
    LED_ST_ON,
    LED_ST_PULSE,     /* горить, після таймауту згасне сам */
    LED_ST_BLINK_ON,  /* блимає, зараз горить */
    LED_ST_BLINK_OFF, /* блимає, зараз не горить */
} LedState;

typedef struct
{
    const LedConfig *cfg;
    LedState state;
} Led;

void led_init(Led *l, const LedConfig *cfg); /* старт: OFF */
void led_on(Led *l);
void led_off(Led *l);
/* Блимати: half_period_ms горить, half_period_ms не горить. Завжди стартує з фази "горить",
 * тому два LED, запущені одним викликом за одним, блимають синхронно. */
void led_blink(Led *l, uint16_t half_period_ms);
/* Загорітися на ms і згаснути. Повторний виклик перезапускає відлік. */
void led_pulse(Led *l, uint16_t ms);

void led_on_timeout(Led *l); /* SIG_TIMEOUT свого таймера */
