#include "led.h"

/* Єдине місце, де пишемо в пін: тут "світиться/ні" перетворюється на SET/RESET. */
static void write(Led *l, bool lit)
{
    l->lit = lit;
    GPIO_PinState inactive = (l->active == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET;
    HAL_GPIO_WritePin(l->port, l->pin, lit ? l->active : inactive);
}

static void start(Led *l, LedMode mode, bool lit, uint32_t period)
{
    l->mode = mode;
    l->period = period;
    l->since = HAL_GetTick();
    write(l, lit);
}

void led_init(Led *l)
{
    led_off(l); /* спершу рівень "вимкнено", потім вихід — LED не блимне на старті */

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = l->pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(l->port, &gpio);
}

void led_on(Led *l) { start(l, LED_ON, true, 0); }
void led_off(Led *l) { start(l, LED_OFF, false, 0); }
void led_blink(Led *l, uint32_t half_period_ms) { start(l, LED_BLINK, true, half_period_ms); }
void led_pulse(Led *l, uint32_t ms) { start(l, LED_PULSE, true, ms); }

void led_update(Led *l)
{
    if (l->mode == LED_OFF || l->mode == LED_ON)
    {
        return; /* тут нічого не змінюється з часом */
    }
    if (HAL_GetTick() - l->since < l->period)
    {
        return; /* ще не час */
    }

    if (l->mode == LED_PULSE)
    {
        led_off(l);
    }
    else /* LED_BLINK */
    {
        l->since += l->period; /* "+=", а не "= now": блимання не збивається з ритму */
        write(l, !l->lit);
    }
}
