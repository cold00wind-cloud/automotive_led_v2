/*
 * main.c — "точка збирання" (composition root).
 *
 * Тут і тільки тут:
 *   1) створюються всі об'єкти (кнопки, LED, замок, авто) і їхні конфіги;
 *   2) працює цикл подій: дістати подію → віддати власнику → спати.
 *
 * Шари: DL (dl/, board.c) → CL (cl/) → BL (bl/). Кожен шар підключає тільки
 * хедери шарів НИЖЧЕ себе. main.c бачить усіх, бо саме він їх з'єднує.
 */
#include "stm32f4xx_hal.h"
#include "app_config.h"
#include "board.h"
#include "dl/event_queue.h"
#include "dl/soft_timer.h"
#include "cl/button.h"
#include "cl/led.h"
#include "cl/lock.h"
#include "bl/car.h"

_Static_assert(TMR_COUNT <= ST_MAX_TIMERS, "збільш ST_MAX_TIMERS у soft_timer.h");

/* ============ Конфіги (const → flash) ============ */
static const ButtonConfig BUTTON_CFG[BTN_COUNT] = {
    [BTN_LOCK] = {
        .pin = &PIN_BTN_LOCK,
        .id = BTN_LOCK,
        .debounce_tmr = TMR_DEBOUNCE_BASE + BTN_LOCK,
        .gesture_tmr = TMR_GESTURE_BASE + BTN_LOCK,
        .debounce_ms = DEBOUNCE_MS,
        .long_ms = LONG_PRESS_MS,
        .double_ms = 0, /* SHORT без затримки на очікування другого натискання */
    },
    [BTN_EMRG] = {
        .pin = &PIN_BTN_EMRG,
        .id = BTN_EMRG,
        .debounce_tmr = TMR_DEBOUNCE_BASE + BTN_EMRG,
        .gesture_tmr = TMR_GESTURE_BASE + BTN_EMRG,
        .debounce_ms = DEBOUNCE_MS,
        .long_ms = LONG_PRESS_MS,
        .double_ms = DOUBLE_CLICK_MS, /* тільки цій кнопці потрібне подвійне натискання */
    },
    [BTN_CRASH] = {
        .pin = &PIN_BTN_CRASH,
        .id = BTN_CRASH,
        .debounce_tmr = TMR_DEBOUNCE_BASE + BTN_CRASH,
        .gesture_tmr = TMR_GESTURE_BASE + BTN_CRASH,
        .debounce_ms = DEBOUNCE_MS,
        .long_ms = LONG_PRESS_MS,
        .double_ms = 0,
    },
};

static const LedConfig LED_CFG[LED_COUNT] = {
    [LED_LEFT] = {.pin = &PIN_LED_LEFT, .timer = TMR_LED_BASE + LED_LEFT},
    [LED_RIGHT] = {.pin = &PIN_LED_RIGHT, .timer = TMR_LED_BASE + LED_RIGHT},
    [LED_STOP] = {.pin = &PIN_LED_STOP, .timer = TMR_LED_BASE + LED_STOP},
    [LED_VALET] = {.pin = &PIN_LED_VALET, .timer = TMR_LED_BASE + LED_VALET},
};

/* ============ Стан (RAM) ============ */
static Button buttons[BTN_COUNT];
static Led leds[LED_COUNT];
static Lock lock;
static Car car;

/* ============ Диспетчер ============
 * Єдине місце, яке знає "хто власник події". Компоненти один одного не
 * викликають — тільки через події та цей маршрутизатор.
 */
static void route_timeout(uint8_t timer_id)
{
    if (timer_id < TMR_GESTURE_BASE)
    {
        button_on_timeout(&buttons[timer_id - TMR_DEBOUNCE_BASE], timer_id);
    }
    else if (timer_id < TMR_LED_BASE)
    {
        button_on_timeout(&buttons[timer_id - TMR_GESTURE_BASE], timer_id);
    }
    else if (timer_id < TMR_COUNT)
    {
        led_on_timeout(&leds[timer_id - TMR_LED_BASE]);
    }
}

static void dispatch(const Event *e)
{
    switch (e->sig)
    {
    case SIG_PIN_EDGE:
        button_on_edge(&buttons[e->id]);
        break;

    case SIG_TIMEOUT:
        /* Таймер перезапустили/зупинили, поки ця подія лежала в черзі → вона застаріла. */
        if (st_is_current(e->id, e->tag))
        {
            route_timeout(e->id);
        }
        break;

    case SIG_BTN_SHORT:
    case SIG_BTN_DOUBLE:
    case SIG_BTN_LONG:
        car_on_button(&car, (ButtonId)e->id, e->sig);
        break;
    }
}

/*
 * Сон до наступного переривання — без гонки.
 * Якщо зробити просто "if (empty) __WFI();", то переривання може прийти МІЖ
 * перевіркою і WFI: подія вже в черзі, а ми заснули і чекаємо наступну.
 * З вимкненими перериваннями WFI однаково прокидається від pending-IRQ,
 * а обробник виконається одразу після __enable_irq().
 */
static void sleep_until_event(void)
{
    __disable_irq();
    if (eq_is_empty())
    {
        __WFI();
    }
    __enable_irq();
}

int main(void)
{
    HAL_Init(); /* серед іншого запускає SysTick на 1 мс (HSI 16 МГц) */
    board_init();

    for (uint8_t id = 0; id < BTN_COUNT; id++)
    {
        button_init(&buttons[id], &BUTTON_CFG[id]);
    }
    for (uint8_t id = 0; id < LED_COUNT; id++)
    {
        led_init(&leds[id], &LED_CFG[id]);
    }
    lock_init(&lock, &PIN_RELAY);
    car_init(&car, (CarParts){
                       .lock = &lock,
                       .left = &leds[LED_LEFT],
                       .right = &leds[LED_RIGHT],
                       .stop = &leds[LED_STOP],
                       .valet = &leds[LED_VALET],
                   });

    for (;;)
    {
        Event e;
        if (eq_pop(&e))
        {
            dispatch(&e);
        }
        else
        {
            sleep_until_event();
        }
    }
}
