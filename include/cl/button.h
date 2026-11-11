#pragma once
/*
 * Button (CL) — перетворює "сирі" фронти на піні в жести: SHORT / DOUBLE / LONG.
 *
 * Два рівні всередині:
 *   1) Debounce: на перший фронт запускаємо таймер (50 мс). Коли сплив —
 *      ОДИН раз читаємо пін. Змінився стабільний рівень → PRESS або RELEASE.
 *   2) Жести: стейт-машина над PRESS / RELEASE / TIMEOUT (таблиця в button.c).
 *
 * Результат кнопка НЕ передає в бізнес-логіку викликом функції, а кладе в
 * чергу подію SIG_BTN_*. Так CL нічого не знає про BL (залежність тільки вниз).
 *
 * Одна реалізація на всі кнопки. Відмінності — у конфігу (double_ms = 0
 * вимикає подвійне натискання, тоді SHORT приходить одразу після відпускання).
 */
#include <stdbool.h>
#include <stdint.h>
#include "dl/gpio_pin.h"

/* КОНФІГ: не змінюється під час роботи, лежить у flash (const). */
typedef struct
{
    const Pin *pin;
    uint8_t id;           /* піде в Event.id подій SIG_BTN_* */
    uint8_t debounce_tmr; /* номер програмного таймера для debounce */
    uint8_t gesture_tmr;  /* номер програмного таймера для long/double */
    uint16_t debounce_ms;
    uint16_t long_ms;
    uint16_t double_ms; /* 0 = подвійне натискання вимкнене */
} ButtonConfig;

typedef enum
{
    BTN_ST_IDLE,         /* відпущена, нічого не чекаємо */
    BTN_ST_PRESSED,      /* натиснута: відпустять (SHORT) чи мине long_ms (LONG)? */
    BTN_ST_WAIT_RELEASE, /* LONG уже відправили, чекаємо відпускання */
    BTN_ST_WAIT_SECOND,  /* відпустили, чекаємо друге натискання протягом double_ms */
    BTN_ST_SECOND_PRESS, /* друге натискання, на відпусканні буде DOUBLE */
} ButtonState;

/* СТАН: змінюється під час роботи, лежить у RAM. */
typedef struct
{
    const ButtonConfig *cfg;
    ButtonState state;
    bool pressed; /* останній стабільний (після debounce) рівень */
} Button;

void button_init(Button *b, const ButtonConfig *cfg);

/* Обробники подій — їх викликає диспетчер у main.c. */
void button_on_edge(Button *b);                      /* SIG_PIN_EDGE */
void button_on_timeout(Button *b, uint8_t timer_id); /* SIG_TIMEOUT одного з двох таймерів */
