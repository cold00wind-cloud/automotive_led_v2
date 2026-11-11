#include "cl/led.h"
#include "dl/soft_timer.h"

/* Стан і пін завжди змінюються разом — в одному місці. */
static void enter(Led *l, LedState s)
{
    l->state = s;
    pin_set_active(l->cfg->pin, s == LED_ST_ON || s == LED_ST_PULSE || s == LED_ST_BLINK_ON);
}

void led_init(Led *l, const LedConfig *cfg)
{
    l->cfg = cfg;
    enter(l, LED_ST_OFF);
}

void led_on(Led *l)
{
    st_stop(l->cfg->timer);
    enter(l, LED_ST_ON);
}

void led_off(Led *l)
{
    st_stop(l->cfg->timer);
    enter(l, LED_ST_OFF);
}

void led_blink(Led *l, uint16_t half_period_ms)
{
    st_start(l->cfg->timer, half_period_ms, true);
    enter(l, LED_ST_BLINK_ON);
}

void led_pulse(Led *l, uint16_t ms)
{
    st_start(l->cfg->timer, ms, false);
    enter(l, LED_ST_PULSE);
}

void led_on_timeout(Led *l)
{
    switch (l->state)
    {
    case LED_ST_PULSE:
        enter(l, LED_ST_OFF);
        break;
    case LED_ST_BLINK_ON:
        enter(l, LED_ST_BLINK_OFF);
        break;
    case LED_ST_BLINK_OFF:
        enter(l, LED_ST_BLINK_ON);
        break;
    case LED_ST_OFF:
    case LED_ST_ON:
        break; /* таймер не працює в цих станах — подія застаріла, ігноруємо */
    }
}
