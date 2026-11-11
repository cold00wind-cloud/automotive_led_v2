#include "bl/car.h"

/* ================= Виходи, обчислені зі стану =================
 * Одне рішення — в одному місці: "що горить" вирішує ТІЛЬКИ ця функція.
 * Аварійка має пріоритет над поворотником; вибір водія (turn) при цьому
 * не губиться і повернеться, коли аварійку вимкнуть.
 */
static void apply_lights(Car *c)
{
    bool want_left = c->hazard || c->turn == TURN_LEFT;
    bool want_right = c->hazard || c->turn == TURN_RIGHT;

    if (want_left == c->shown_left && want_right == c->shown_right)
    {
        return; /* нічого не змінилось — не збиваємо фазу блимання */
    }
    c->shown_left = want_left;
    c->shown_right = want_right;

    /* Перезапускаємо ОБИДВА разом: тоді аварійка блимає синхронно. */
    if (want_left)
        led_blink(c->parts.left, BLINK_MS);
    else
        led_off(c->parts.left);

    if (want_right)
        led_blink(c->parts.right, BLINK_MS);
    else
        led_off(c->parts.right);
}

static void apply_valet_led(Car *c)
{
    if (c->mode == CAR_VALET)
        led_on(c->parts.valet);
    else
        led_off(c->parts.valet);
}

/* ================= Переходи між режимами ================= */
static void enter_normal(Car *c)
{
    c->mode = CAR_NORMAL;
    /* Замок лишається відкритим — закрити можна коротким натисканням. */
}

static void enter_valet(Car *c)
{
    c->mode = CAR_VALET;
    lock_open(c->parts.lock);
}

static void enter_crash(Car *c)
{
    c->mode = CAR_CRASH;
    lock_open(c->parts.lock); /* "відкриває двері" */
    c->hazard = true;
}

/* ================= Реакції на кнопки ================= */
static Turn toggle_turn(Turn current, Turn wanted)
{
    return (current == wanted) ? TURN_OFF : wanted;
}

static void on_lock(Car *c, Signal g)
{
    switch (c->mode)
    {
    case CAR_NORMAL:
        if (g == SIG_BTN_SHORT)
            lock_toggle(c->parts.lock);
        else if (g == SIG_BTN_LONG)
            enter_valet(c);
        break;
    case CAR_VALET:
        if (g == SIG_BTN_LONG)
            enter_normal(c);
        /* SHORT ігноруємо: у valet замок заблоковано у відкритому стані */
        break;
    case CAR_CRASH:
        if (g == SIG_BTN_LONG)
        {
            c->hazard = false; /* crash знімається тільки valet-режимом */
            enter_valet(c);
        }
        break;
    }
}

static void on_emrg(Car *c, Signal g)
{
    if (c->mode == CAR_CRASH)
    {
        return; /* в аварії аварійку не вимкнути */
    }
    switch (g)
    {
    case SIG_BTN_SHORT:
        c->turn = toggle_turn(c->turn, TURN_LEFT);
        break;
    case SIG_BTN_DOUBLE:
        c->turn = toggle_turn(c->turn, TURN_RIGHT);
        break;
    case SIG_BTN_LONG:
        c->hazard = !c->hazard;
        break;
    default:
        break;
    }
}

static void on_crash_sensor(Car *c, Signal g)
{
    if (c->mode == CAR_CRASH)
    {
        return;
    }
    if (g == SIG_BTN_SHORT)
        led_pulse(c->parts.stop, BRAKE_MS); /* "пригальмовує" */
    else if (g == SIG_BTN_LONG)
        enter_crash(c);
}

/* ================= Публічне API ================= */
void car_init(Car *c, CarParts parts)
{
    c->parts = parts;
    c->mode = CAR_NORMAL;
    c->turn = TURN_OFF;
    c->hazard = false;
    c->shown_left = false;
    c->shown_right = false;
    apply_valet_led(c);
}

void car_on_button(Car *c, ButtonId button, Signal gesture)
{
    switch (button)
    {
    case BTN_LOCK:
        on_lock(c, gesture);
        break;
    case BTN_EMRG:
        on_emrg(c, gesture);
        break;
    case BTN_CRASH:
        on_crash_sensor(c, gesture);
        break;
    case BTN_COUNT:
        break;
    }
    /* Після будь-якої зміни стану — привести виходи у відповідність. */
    apply_lights(c);
    apply_valet_led(c);
}
