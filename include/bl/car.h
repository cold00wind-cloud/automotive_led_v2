#pragma once
/*
 * Car (BL) — бізнес-логіка: ЩО робить машина у відповідь на жести кнопок.
 *
 * BL не знає про піни, debounce чи переривання. Вхід — "кнопка X зробила
 * жест Y", вихід — команди компонентам CL (lock_open, led_blink, ...).
 *
 * Стан розкладено на НЕЗАЛЕЖНІ частини, кожна — свій enum/bool:
 *   mode   — NORMAL / VALET / CRASH (головна стейт-машина)
 *   turn   — OFF / LEFT / RIGHT     (що вибрав водій)
 *   hazard — аварійка ввімкнена?
 * Те, що бачимо на LED, НЕ зберігається окремо як "правда" — воно
 * обчислюється з цих полів (apply_lights у car.c).
 */
#include <stdbool.h>
#include "app_config.h"
#include "cl/led.h"
#include "cl/lock.h"
#include "dl/event.h"

typedef enum
{
    CAR_NORMAL,
    CAR_VALET, /* замок відкритий, кнопка замка коротко не працює */
    CAR_CRASH, /* після сильного удару: тільки valet може вивести звідси */
} CarMode;

typedef enum
{
    TURN_OFF,
    TURN_LEFT,
    TURN_RIGHT,
} Turn;

/* Компоненти, якими керує BL. Їх створює main.c і передає сюди вказівники. */
typedef struct
{
    Lock *lock;
    Led *left;
    Led *right;
    Led *stop;
    Led *valet;
} CarParts;

typedef struct
{
    CarParts parts;
    CarMode mode;
    Turn turn;
    bool hazard;
    /* Що зараз виведено на поворотники — щоб не перезапускати блимання без потреби. */
    bool shown_left;
    bool shown_right;
} Car;

void car_init(Car *c, CarParts parts);
void car_on_button(Car *c, ButtonId button, Signal gesture);
