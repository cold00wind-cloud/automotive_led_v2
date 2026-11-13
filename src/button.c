#include "button.h"
#include "config.h"

void button_init(Button *b)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = b->pin;
    gpio.Mode = GPIO_MODE_IT_RISING_FALLING;
    /* Підтяжка тримає пін у НЕактивному рівні, поки кнопку не натиснули. */
    gpio.Pull = (b->active == GPIO_PIN_RESET) ? GPIO_PULLUP : GPIO_PULLDOWN;
    HAL_GPIO_Init(b->port, &gpio);

    /* Якщо кнопку тримають під час старту — це не рахується натисканням. */
    b->pressed = HAL_GPIO_ReadPin(b->port, b->pin) == b->active;
    b->state = BTN_IDLE;
}

void button_on_edge(Button *b)
{
    b->edge = true;
    b->edge_time = HAL_GetTick();
}

ButtonEvent button_update(Button *b)
{
    uint32_t now = HAL_GetTick();

    /* ---- 1. Брязкіт: віримо новому рівню, тільки якщо він тримається DEBOUNCE_MS ---- */
    bool press = false;   /* щойно натиснули */
    bool release = false; /* щойно відпустили */

    __disable_irq();
    bool settled = b->edge && (HAL_GetTick() - b->edge_time >= DEBOUNCE_MS);
    if (settled)
        b->edge = false;
    __enable_irq();
    if (settled)
    {
        bool raw = HAL_GPIO_ReadPin(b->port, b->pin) == b->active;
        if (raw != b->pressed)
        {
            b->pressed = raw;
            press = raw;
            release = !raw;
        }
    }

    switch (b->state)
    {
    case BTN_IDLE:
        if (press)
        {
            b->state = BTN_PRESSED;
            b->state_time = now;
        }
        break;

    case BTN_PRESSED:
        if (release && b->double_ms == 0u)
        {
            b->state = BTN_IDLE;
            return EVT_SHORT; /* подвійне вимкнене — не чекаємо, одразу коротке */
        }
        if (release)
        {
            b->state = BTN_WAIT_SECOND;
            b->state_time = now;
        }
        else if (now - b->state_time >= LONG_PRESS_MS)
        {
            b->state = BTN_WAIT_RELEASE;
            return EVT_LONG;
        }
        break;

    case BTN_WAIT_SECOND:
        if (press)
        {
            b->state = BTN_SECOND_PRESS;
        }
        else if (now - b->state_time >= b->double_ms)
        {
            b->state = BTN_IDLE;
            return EVT_SHORT; /* другого натискання не дочекалися */
        }
        break;

    case BTN_SECOND_PRESS:
        if (release)
        {
            b->state = BTN_IDLE;
            return EVT_DOUBLE;
        }
        break;

    case BTN_WAIT_RELEASE:
        if (release)
        {
            b->state = BTN_IDLE;
        }
        break;
    }
    return EVT_NONE;
}
