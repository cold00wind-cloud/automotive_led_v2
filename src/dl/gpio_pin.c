#include "dl/gpio_pin.h"
#include "dl/critical.h"

static GPIO_PinState level_for(const Pin *p, bool active)
{
    if (active)
    {
        return p->active;
    }
    return (p->active == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET;
}

void pin_init_output(const Pin *p, uint32_t mode)
{
    HAL_GPIO_WritePin(p->port, p->pin, level_for(p, false));

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = p->pin;
    gpio.Mode = mode;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(p->port, &gpio);
}

void pin_init_irq_input(const Pin *p)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = p->pin;
    gpio.Mode = GPIO_MODE_IT_RISING_FALLING; /* потрібні обидва фронти: натиснули І відпустили */
    gpio.Pull = (p->active == GPIO_PIN_RESET) ? GPIO_PULLUP : GPIO_PULLDOWN;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(p->port, &gpio);
}

bool pin_is_active(const Pin *p)
{
    return HAL_GPIO_ReadPin(p->port, p->pin) == p->active;
}

void pin_set_active(const Pin *p, bool active)
{
    HAL_GPIO_WritePin(p->port, p->pin, level_for(p, active));
}

/*
 * EXTI->IMR — спільний регістр для всіх ліній. "|=" і "&=" — це
 * прочитати-змінити-записати; якщо між читанням і записом спрацює ISR іншої
 * кнопки і теж змінить IMR, одна зі змін загубиться. Тому — критична секція.
 */
void pin_irq_mask(const Pin *p)
{
    uint32_t primask = critical_enter();
    EXTI->IMR &= ~(uint32_t)p->pin;
    critical_exit(primask);
}

void pin_irq_unmask(const Pin *p)
{
    uint32_t primask = critical_enter();
    EXTI->PR = p->pin; /* скинути фронти, що накопичились поки лінія була замаскована (запис 1 = скинути) */
    EXTI->IMR |= p->pin;
    critical_exit(primask);
}
