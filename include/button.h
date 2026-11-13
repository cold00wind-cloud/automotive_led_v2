#pragma once
/*
 * Кнопка: прибирає брязкіт контактів і розпізнає жести
 * (коротке / подвійне / довге натискання).
 *
 */
#include <stdbool.h>
#include "stm32f4xx_hal.h"

typedef enum
{
    EVT_NONE,   /* нічого не сталося */
    EVT_SHORT,  /* коротке натискання */
    EVT_DOUBLE, /* подвійне натискання */
    EVT_LONG,   /* утримання довше за LONG_PRESS_MS */
} ButtonEvent;

typedef enum
{
    BTN_IDLE,         /* відпущена, нічого не чекаємо */
    BTN_PRESSED,      /* натиснута: відпустять (коротке) чи дотримають (довге)? */
    BTN_WAIT_RELEASE, /* довге вже повідомили, чекаємо відпускання */
    BTN_WAIT_SECOND,  /* відпустили, чекаємо друге натискання */
    BTN_SECOND_PRESS, /* друге натискання: на відпусканні буде подвійне */
} ButtonState;

typedef struct
{
    /* Налаштування — заповнюєш сам при створенні */
    GPIO_TypeDef *port;
    uint16_t pin;
    GPIO_PinState active; /* рівень на піні, коли кнопка натиснута */
    uint16_t double_ms;   /* вікно подвійного натискання; 0 = подвійне вимкнене */

    /* Стан — веде сама кнопка */
    bool pressed;       /* натиснута чи ні (вже без брязкоту) */
    ButtonState state;
    uint32_t state_time; /* коли увійшли в поточний стан */
    volatile bool edge;
    volatile uint32_t edge_time;
} Button;

void button_init(Button *b);
void button_on_edge(Button *b);

/* Викликати в кожному проході головного циклу. Повертає жест або EVT_NONE. */
ButtonEvent button_update(Button *b);