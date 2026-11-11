#include "board.h"
#include "app_config.h"
#include "dl/event_queue.h"
#include "dl/soft_timer.h"

/* ================= Піни =================
 * Кожна кнопка — на СВОЄМУ номері піна: лінія EXTI x може слухати тільки
 * один порт (PAx або PBx або ...). Тому PA0 і PB0 разом працювати не можуть,
 * і аварійку перенесено з PB0 на PB4.
 */
const Pin PIN_BTN_LOCK = {GPIOA, GPIO_PIN_0, GPIO_PIN_RESET}; /* KEY на Black Pill: замикає на GND */
const Pin PIN_BTN_EMRG = {GPIOB, GPIO_PIN_4, GPIO_PIN_SET};   /* кнопка на +3.3V */
const Pin PIN_BTN_CRASH = {GPIOB, GPIO_PIN_1, GPIO_PIN_SET};  /* датчик удару на +3.3V */

/* ПРИПУЩЕННЯ: LED на PA1..PA3 світяться від одиниці. Якщо в тебе навпаки — заміни SET на RESET. */
const Pin PIN_LED_STOP = {GPIOA, GPIO_PIN_1, GPIO_PIN_SET};
const Pin PIN_LED_LEFT = {GPIOA, GPIO_PIN_2, GPIO_PIN_SET};
const Pin PIN_LED_RIGHT = {GPIOA, GPIO_PIN_3, GPIO_PIN_SET};
const Pin PIN_LED_VALET = {GPIOC, GPIO_PIN_13, GPIO_PIN_RESET}; /* синій LED Black Pill — активний нуль */

/* ПРИПУЩЕННЯ: реле спрацьовує, коли open-drain тягне в нуль; відпущене реле = замок закритий. */
const Pin PIN_RELAY = {GPIOB, GPIO_PIN_12, GPIO_PIN_RESET};

/* Таблиця "ButtonId → пін" для ISR. */
static const Pin *const BUTTON_PINS[BTN_COUNT] = {
    [BTN_LOCK] = &PIN_BTN_LOCK,
    [BTN_EMRG] = &PIN_BTN_EMRG,
    [BTN_CRASH] = &PIN_BTN_CRASH,
};

void board_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE(); /* SYSCFG вибирає порт для лінії EXTI (потрібно для PB1, PB4) */

    pin_init_output(&PIN_RELAY, GPIO_MODE_OUTPUT_OD);
    pin_init_output(&PIN_LED_STOP, GPIO_MODE_OUTPUT_PP);
    pin_init_output(&PIN_LED_LEFT, GPIO_MODE_OUTPUT_PP);
    pin_init_output(&PIN_LED_RIGHT, GPIO_MODE_OUTPUT_PP);
    pin_init_output(&PIN_LED_VALET, GPIO_MODE_OUTPUT_PP);

    for (uint8_t id = 0; id < BTN_COUNT; id++)
    {
        pin_init_irq_input(BUTTON_PINS[id]);
    }

    HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
    HAL_NVIC_SetPriority(EXTI1_IRQn, 2, 0);
    HAL_NVIC_SetPriority(EXTI4_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
}

/* ================= Переривання =================
 * Правило: ISR нічого не вирішує. Він тільки фіксує факт і кладе подію в чергу.
 */

void SysTick_Handler(void)
{
    HAL_IncTick(); /* потрібно самому HAL */
    st_tick();     /* наші програмні таймери */
}

void EXTI0_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0); }
void EXTI1_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1); }
void EXTI4_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4); }

/* HAL викликає це з EXTIx_IRQHandler вище (прапорець PR він уже скинув). */
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    for (uint8_t id = 0; id < BTN_COUNT; id++)
    {
        if (BUTTON_PINS[id]->pin == pin)
        {
            /* Глушимо лінію до кінця debounce: решта "брязкоту" не засмічує чергу.
             * Розглушить компонент Button, коли сплине debounce-таймер. */
            pin_irq_mask(BUTTON_PINS[id]);
            eq_post((Event){.sig = SIG_PIN_EDGE, .id = id, .tag = 0});
            return;
        }
    }
}
