#include "car.h"
#include "config.h"
#include "led.h"

/* ================= Залізо ================= */
/* LED: порт, пін, рівень, при якому світиться. Якщо твій LED активний нулем — заміни SET на RESET. */
static Led led_left = {.port = GPIOA, .pin = GPIO_PIN_2, .active = GPIO_PIN_SET};
static Led led_right = {.port = GPIOA, .pin = GPIO_PIN_3, .active = GPIO_PIN_SET};
static Led led_stop = {.port = GPIOA, .pin = GPIO_PIN_6, .active = GPIO_PIN_SET};
static Led led_valet = {.port = GPIOC, .pin = GPIO_PIN_13, .active = GPIO_PIN_RESET}; /* синій LED на Black Pill */

/* Реле замка: open-drain, спрацьовує від нуля. Відпущене реле = замок закритий. */
#define RELAY_PORT GPIOB
#define RELAY_PIN  GPIO_PIN_12

/* ================= Стан ================= */
typedef enum
{
    MODE_NORMAL,
    MODE_VALET, /* замок відкритий, коротке натискання замка не працює */
    MODE_CRASH, /* після сильного удару; вийти можна тільки через valet */
} Mode;

typedef enum
{
    TURN_OFF,
    TURN_LEFT,
    TURN_RIGHT,
} Turn;

static Mode mode;
static Turn turn;      /* що вибрав водій */
static bool hazard;    /* аварійка */
static bool lock_open; /* замок відкритий? */

/* ================= Виходи ================= */
static void set_lock(bool open)
{
    lock_open = open;
    HAL_GPIO_WritePin(RELAY_PORT, RELAY_PIN, open ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

/* Що горить — вирішується тільки тут, зі стану (mode, turn, hazard).
 * Аварійка перекриває поворотник, але вибір водія не губиться. */
static void update_lights(void)
{
    if (mode == MODE_VALET)
        led_on(&led_valet);
    else
        led_off(&led_valet);

    bool left = hazard || turn == TURN_LEFT;
    bool right = hazard || turn == TURN_RIGHT;

    bool left_now = led_left.mode == LED_BLINK;
    bool right_now = led_right.mode == LED_BLINK;
    if (left == left_now && right == right_now)
    {
        return; /* нічого не змінилося — не збиваємо ритм блимання */
    }

    /* Запускаємо обидва разом, щоб аварійка блимала синхронно. */
    if (left)
        led_blink(&led_left, BLINK_MS);
    else
        led_off(&led_left);

    if (right)
        led_blink(&led_right, BLINK_MS);
    else
        led_off(&led_right);
}

/* ================= Реакції на кнопки ================= */
static void on_lock(ButtonEvent e)
{
    if (mode == MODE_NORMAL && e == EVT_SHORT)
    {
        set_lock(!lock_open);
    }
    else if (mode == MODE_NORMAL && e == EVT_LONG)
    {
        mode = MODE_VALET;
        set_lock(true);
    }
    else if (mode == MODE_VALET && e == EVT_LONG)
    {
        mode = MODE_NORMAL; /* замок лишається відкритим, закрити — коротким натисканням */
    }
    else if (mode == MODE_CRASH && e == EVT_LONG)
    {
        hazard = false;
        mode = MODE_VALET;
        set_lock(true);
    }
}

static void on_emrg(ButtonEvent e)
{
    if (mode == MODE_CRASH)
    {
        return; /* після аварії аварійку не вимкнути */
    }
    if (e == EVT_SHORT)
        turn = (turn == TURN_LEFT) ? TURN_OFF : TURN_LEFT;
    else if (e == EVT_DOUBLE)
        turn = (turn == TURN_RIGHT) ? TURN_OFF : TURN_RIGHT;
    else if (e == EVT_LONG)
        hazard = !hazard;
}

static void on_crash(ButtonEvent e)
{
    if (mode == MODE_CRASH)
    {
        return;
    }
    if (e == EVT_SHORT)
    {
        led_pulse(&led_stop, BRAKE_MS); /* легкий удар — "пригальмовує" */
    }
    else if (e == EVT_LONG)
    {
        mode = MODE_CRASH; /* сильний удар: відкрити двері, увімкнути аварійку */
        set_lock(true);
        hazard = true;
    }
}

/* ================= Публічні функції ================= */
void car_init(void)
{
    led_init(&led_left);
    led_init(&led_right);
    led_init(&led_stop);
    led_init(&led_valet);

    set_lock(false); /* спершу рівень "закрито", потім вихід — реле не клацне на старті */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = RELAY_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RELAY_PORT, &gpio);

    mode = MODE_NORMAL;
    turn = TURN_OFF;
    hazard = false;
}

void car_on_button(ButtonId button, ButtonEvent event)
{
    if (button == BTN_LOCK)
        on_lock(event);
    else if (button == BTN_EMRG)
        on_emrg(event);
    else if (button == BTN_CRASH)
        on_crash(event);

    update_lights();
}

void car_update(void)
{
    led_update(&led_left);
    led_update(&led_right);
    led_update(&led_stop);
}
