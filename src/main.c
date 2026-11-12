/*
 * Головний цикл: раз на 1 мс опитати кнопки, передати жести в логіку машини,
 * оновити світлодіоди.
 *
 *   main.c   — кнопки і цикл
 *   button.c — брязкіт + жести (коротке / подвійне / довге)
 *   led.c    — увімкнути / вимкнути / блимати / спалахнути
 *   car.c    — що машина робить у відповідь на жести
 *   config.h — усі часи в мілісекундах
 */
#include "stm32f4xx_hal.h"
#include "button.h"
#include "car.h"
#include "config.h"

/* Кнопки: порт, пін, рівень коли натиснута, вікно подвійного натискання (0 = вимкнене). */
static Button buttons[BTN_COUNT] = {
    [BTN_LOCK] = {.port = GPIOA, .pin = GPIO_PIN_0, .active = GPIO_PIN_RESET, .double_ms = 0},              /* KEY на Black Pill, замикає на GND */
    [BTN_EMRG] = {.port = GPIOB, .pin = GPIO_PIN_4, .active = GPIO_PIN_SET, .double_ms = DOUBLE_CLICK_MS}, /* кнопка на +3.3V */
    [BTN_CRASH] = {.port = GPIOB, .pin = GPIO_PIN_1, .active = GPIO_PIN_SET, .double_ms = 0},              /* датчик удару на +3.3V */
};

/* Раз на 1 мс: HAL рахує час для HAL_GetTick(). */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

int main(void)
{
    HAL_Init(); /* серед іншого запускає SysTick на 1 мс */

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    for (int i = 0; i < BTN_COUNT; i++)
    {
        button_init(&buttons[i]);
    }
    car_init();

    for (;;)
    {
        for (int i = 0; i < BTN_COUNT; i++)
        {
            ButtonEvent event = button_update(&buttons[i]);
            if (event != EVT_NONE)
            {
                car_on_button((ButtonId)i, event);
            }
        }
        car_update();

        __WFI(); /* спимо до наступного переривання SysTick, тобто до 1 мс */
    }
}
