
#include "stm32f4xx_hal.h" /* Бібліотека HAL для мікроконтролерів серії STM32F4 */
#include "stdbool.h"
#include "Led.h"

// =========== C port =======
#define WALET_PORT GPIOC
#define LED_WALET_PIN GPIO_PIN_13

// =========== B port =======
#define B_PORT GPIOB
#define RELAY_PIN GPIO_PIN_12
#define BTN_EMRG_PIN GPIO_PIN_0
#define BTN_CRSH_PIN GPIO_PIN_1

// =========== A port =======
#define A_PORT GPIOA
#define BTN_KEY_PIN GPIO_PIN_0
#define BTN_IRQn EXTI0_IRQn
#define LED_STOP_PIN GPIO_PIN_1
#define LED_LEFT_PIN GPIO_PIN_2
#define LED_RIGHT_PIN GPIO_PIN_3

#define DEBOUNCE_MS 50

typedef enum
{
    IDLE,
    PRESS,
    LONG_HOLD,
    WAIT_SECOND,
    SECOND_PRESS
} BtnStates;

typedef enum 
{
    EVT_NONE,
    EVT_SHORT,
    EVT_DOUBLE,
    EVT_LONG,
} BtnEvents;

TIM_HandleTypeDef htim2;

void static GPIO_Init(void);
void static TIM_Init(void);
void Error_Handler(void);

int main(void)
{
    HAL_Init();
    GPIO_Init();
    TIM_Init();

    while (1)
    {
        __WFI();
    }
}

void GPIO_Init()
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    HAL_GPIO_WritePin(B_PORT, RELAY_PIN, GPIO_PIN_SET);
    
    // === Setup Rele
    gpio.Pin = RELAY_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(B_PORT, &gpio);
    
    // === Setup LEDs
    gpio.Pin = LED_LEFT_PIN | LED_RIGHT_PIN | LED_STOP_PIN ;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(A_PORT, &gpio);
    HAL_GPIO_WritePin(A_PORT, LED_LEFT_PIN | LED_RIGHT_PIN | LED_STOP_PIN, GPIO_PIN_SET);
    
    gpio.Pin = LED_WALET_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(WALET_PORT, &gpio);
    HAL_GPIO_WritePin(WALET_PORT, LED_WALET_PIN, GPIO_PIN_RESET);
    
    // === Setup Buttons
    gpio.Pin = BTN_KEY_PIN;
    gpio.Mode = GPIO_MODE_IT_FALLING;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(A_PORT, &gpio);

    gpio.Pin = BTN_EMRG_PIN;
    gpio.Mode = GPIO_MODE_IT_RISING;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(B_PORT, &gpio);

    gpio.Pin = BTN_CRSH_PIN;
    gpio.Mode = GPIO_MODE_IT_RISING;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(B_PORT, &gpio);


    HAL_NVIC_SetPriority(BTN_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(BTN_IRQn);
}

void TIM_Init()
{
    __HAL_RCC_TIM2_CLK_ENABLE();
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 16000 - 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = DEBOUNCE_MS - 1;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim2);
}

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(BTN_KEY_PIN);
}

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{

    if (BTN_KEY_PIN == pin)
    {

        HAL_NVIC_DisableIRQ(BTN_IRQn);

        __HAL_TIM_SetCounter(&htim2, 0);
        __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);

        HAL_TIM_Base_Start_IT(&htim2);
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (TIM2 == htim->Instance)
    {
        HAL_TIM_Base_Stop_IT(&htim2);

        if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(A_PORT, BTN_KEY_PIN))
        {
            HAL_GPIO_TogglePin(B_PORT, RELAY_PIN);
        }

        __HAL_GPIO_EXTI_CLEAR_IT(BTN_KEY_PIN);
        HAL_NVIC_ClearPendingIRQ(BTN_IRQn);

        HAL_NVIC_EnableIRQ(BTN_IRQn);
    }
}

void Error_Handler(void)
{
    __disable_irq(); /* Глобально забороняємо всі переривання */
    while (1)
    {
        /* Нескінченний цикл — програма тут зупиняється */
    }
}