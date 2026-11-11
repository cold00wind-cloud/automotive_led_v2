#include "cl/button.h"
#include "dl/event_queue.h"
#include "dl/soft_timer.h"

/* Вхідні сигнали стейт-машини жестів (внутрішні, назовні не видно). */
typedef enum
{
    IN_PRESS,
    IN_RELEASE,
    IN_TIMEOUT,
} GestureInput;

static void emit(const Button *b, Signal sig)
{
    eq_post((Event){.sig = sig, .id = b->cfg->id, .tag = 0});
}

/*
 * Стейт-машина жестів.
 *
 *  Стан          | Вхід    | Дія                                  → Новий стан
 *  --------------+---------+--------------------------------------+--------------
 *  IDLE          | PRESS   | таймер long_ms                       → PRESSED
 *  PRESSED       | TIMEOUT | LONG                                 → WAIT_RELEASE
 *  PRESSED       | RELEASE | double вимк: SHORT                   → IDLE
 *                |         | double увімк: таймер double_ms       → WAIT_SECOND
 *  WAIT_SECOND   | TIMEOUT | SHORT                                → IDLE
 *  WAIT_SECOND   | PRESS   | стоп таймер                          → SECOND_PRESS
 *  SECOND_PRESS  | RELEASE | DOUBLE                               → IDLE
 *  WAIT_RELEASE  | RELEASE |                                      → IDLE
 *  інші пари — ігноруються.
 */
static void gesture(Button *b, GestureInput in)
{
    const ButtonConfig *cfg = b->cfg;

    switch (b->state)
    {
    case BTN_ST_IDLE:
        if (in == IN_PRESS)
        {
            st_start(cfg->gesture_tmr, cfg->long_ms, false);
            b->state = BTN_ST_PRESSED;
        }
        break;

    case BTN_ST_PRESSED:
        if (in == IN_TIMEOUT)
        {
            emit(b, SIG_BTN_LONG);
            b->state = BTN_ST_WAIT_RELEASE;
        }
        else if (in == IN_RELEASE && cfg->double_ms == 0u)
        {
            st_stop(cfg->gesture_tmr);
            emit(b, SIG_BTN_SHORT);
            b->state = BTN_ST_IDLE;
        }
        else if (in == IN_RELEASE)
        {
            st_start(cfg->gesture_tmr, cfg->double_ms, false);
            b->state = BTN_ST_WAIT_SECOND;
        }
        break;

    case BTN_ST_WAIT_SECOND:
        if (in == IN_TIMEOUT)
        {
            emit(b, SIG_BTN_SHORT);
            b->state = BTN_ST_IDLE;
        }
        else if (in == IN_PRESS)
        {
            st_stop(cfg->gesture_tmr);
            b->state = BTN_ST_SECOND_PRESS;
        }
        break;

    case BTN_ST_SECOND_PRESS:
        if (in == IN_RELEASE)
        {
            emit(b, SIG_BTN_DOUBLE);
            b->state = BTN_ST_IDLE;
        }
        break;

    case BTN_ST_WAIT_RELEASE:
        if (in == IN_RELEASE)
        {
            b->state = BTN_ST_IDLE;
        }
        break;
    }
}

void button_init(Button *b, const ButtonConfig *cfg)
{
    b->cfg = cfg;
    b->state = BTN_ST_IDLE;
    /* Якщо кнопку тримають під час старту — запам'ятаємо це, і її відпускання
     * не стане "натисканням" (IDLE ігнорує RELEASE). */
    b->pressed = pin_is_active(cfg->pin);
}

void button_on_edge(Button *b)
{
    /* ISR уже заглушив лінію. Чекаємо, поки контакти заспокояться. */
    st_start(b->cfg->debounce_tmr, b->cfg->debounce_ms, false);
}

static void on_debounce_done(Button *b)
{
    /* Порядок важливий: СПОЧАТКУ розглушити лінію, ПОТІМ читати пін.
     * Якщо рівень зміниться між цими кроками — прийде новий фронт, і ми його
     * не пропустимо. У зворотному порядку зміна "провалилася" б у щілину. */
    pin_irq_unmask(b->cfg->pin);

    bool now = pin_is_active(b->cfg->pin);
    if (now == b->pressed)
    {
        return; /* брязкіт або імпульс коротший за debounce — нічого не сталося */
    }
    b->pressed = now;
    gesture(b, now ? IN_PRESS : IN_RELEASE);
}

void button_on_timeout(Button *b, uint8_t timer_id)
{
    if (timer_id == b->cfg->debounce_tmr)
    {
        on_debounce_done(b);
    }
    else if (timer_id == b->cfg->gesture_tmr)
    {
        gesture(b, IN_TIMEOUT);
    }
}
