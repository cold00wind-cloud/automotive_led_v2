#include "car.h"
#include "config.h"
#include "button.h"
#include "led.h"

/* ================= Залізо ================= */
#define BTN_LOCK_IRQn EXTI0_IRQn
#define BTN_CRASH_IRQn EXTI1_IRQn
#define BTN_EMRG_IRQn EXTI4_IRQn

#define BTN_LOCK_PIN GPIO_PIN_0
#define BTN_CRASH_PIN GPIO_PIN_1
#define BTN_EMRG_PIN GPIO_PIN_4

/* LED: порт, пін, рівень, при якому світиться. Якщо твій LED активний нулем — заміни SET на RESET. */
static Led led_left = {.port = GPIOA, .pin = GPIO_PIN_2, .active = GPIO_PIN_SET};
static Led led_right = {.port = GPIOA, .pin = GPIO_PIN_3, .active = GPIO_PIN_SET};
static Led led_stop = {.port = GPIOA, .pin = GPIO_PIN_6, .active = GPIO_PIN_SET};
static Led led_valet = {.port = GPIOC, .pin = GPIO_PIN_13, .active = GPIO_PIN_RESET}; /* синій LED на Black Pill */

/* Кнопки: порт, пін, рівень коли натиснута, вікно подвійного натискання (0 = вимкнене). */
static Button btn_lock = {.port = GPIOA, .pin = BTN_LOCK_PIN, .active = GPIO_PIN_RESET, .double_ms = 0};             /* KEY на Black Pill, замикає на GND */
static Button btn_crash = {.port = GPIOB, .pin = BTN_CRASH_PIN, .active = GPIO_PIN_SET, .double_ms = 0};             /* датчик удару на +3.3V */
static Button btn_emrg = {.port = GPIOB, .pin = BTN_EMRG_PIN, .active = GPIO_PIN_SET, .double_ms = DOUBLE_CLICK_MS}; /* кнопка на +3.3V */

/* Реле замка: open-drain, спрацьовує від нуля. Відпущене реле = замок закритий. */
#define RELAY_PORT GPIOB
#define RELAY_PIN GPIO_PIN_12

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
    switch (mode)
    {
    case MODE_NORMAL:
        if (e == EVT_SHORT)
            set_lock(!lock_open);
        if (e == EVT_LONG)
        {
            mode = MODE_VALET;
            set_lock(true);
        }
        break;

    case MODE_VALET:
        if (e == EVT_LONG)
            mode = MODE_NORMAL;
        break;

    case MODE_CRASH:
        if (e == EVT_LONG)
        {
            hazard = false;
            mode = MODE_VALET;
            set_lock(true);
        }
        break;

    default:
        break;
    }
}

static void on_emrg(ButtonEvent e)
{
    if (mode == MODE_CRASH)
        return; /* після аварії аварійку не вимкнути */
    switch (e)
    {
    case EVT_SHORT:
        turn = (turn == TURN_LEFT) ? TURN_OFF : TURN_LEFT;
        break;
    case EVT_DOUBLE:
        turn = (turn == TURN_RIGHT) ? TURN_OFF : TURN_RIGHT;
        break;
    case EVT_LONG:
        hazard = !hazard;
        break;
    default:
        break;
    }
}

static void on_crash(ButtonEvent e)
{
    if (mode == MODE_CRASH)
        return; /* після аварії аварійку не вимкнути */
    switch (e)
    {
    case EVT_SHORT:
        led_pulse(&led_stop, BRAKE_MS);
        break;
    case EVT_LONG:
        mode = MODE_CRASH; /* сильний удар: відкрити двері, увімкнути аварійку */
        set_lock(true);
        hazard = true;
        break;

    default:
        break;
    }
}

/* ================= Публічні функції ================= */
void car_init(void)
{
    led_init(&led_left);
    led_init(&led_right);
    led_init(&led_stop);
    led_init(&led_valet);

    button_init(&btn_lock);
    button_init(&btn_crash);
    button_init(&btn_emrg);

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

    HAL_NVIC_SetPriority(BTN_LOCK_IRQn, 2, 1);
    HAL_NVIC_SetPriority(BTN_EMRG_IRQn, 2, 2);
    HAL_NVIC_SetPriority(BTN_CRASH_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(BTN_LOCK_IRQn);
    HAL_NVIC_EnableIRQ(BTN_EMRG_IRQn);
    HAL_NVIC_EnableIRQ(BTN_CRASH_IRQn);
}

void car_on_button(ButtonId button, ButtonEvent event)
{
    switch (button)
    {
    case BTN_LOCK:
        on_lock(event);
        break;
    case BTN_EMRG:
        on_emrg(event);
        break;
    case BTN_CRASH:
        on_crash(event);
        break;
    default:
        break;
    }

    update_lights();
}

static void poll_button(Button *b, ButtonId id)
{
    ButtonEvent e = button_update(b);
    if (e != EVT_NONE)
        car_on_button(id, e);
}

void car_update(void)
{
    poll_button(&btn_lock, BTN_LOCK);
    poll_button(&btn_crash, BTN_CRASH);
    poll_button(&btn_emrg, BTN_EMRG);
    
    led_update(&led_left);
    led_update(&led_right);
    led_update(&led_stop);
}

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(btn_lock.pin);
}

void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(btn_crash.pin);
}

void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(btn_emrg.pin);
}

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    switch (pin)
    {
    case BTN_LOCK_PIN:
        button_on_edge(&btn_lock);
        break;
    case BTN_CRASH_PIN:
        button_on_edge(&btn_crash);
        break;
    case BTN_EMRG_PIN:
        button_on_edge(&btn_emrg);
        break;
    default:
        break;
    }
}

// for (int i = 0; i < BTN_COUNT; i++)
// {
//     ButtonEvent event = button_update(&buttons[i]);
//     if (event != EVT_NONE)
//     {
//         car_on_button((ButtonId)i, event);
//     }
// }