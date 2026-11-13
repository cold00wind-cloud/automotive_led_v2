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
#include "car.h"
#include "config.h"

TIM_HandleTypeDef htim2;

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


    car_init();

    while (1)
    {

        car_update();

        __WFI(); /* спимо до наступного переривання SysTick, тобто до 1 мс */
    }
}


