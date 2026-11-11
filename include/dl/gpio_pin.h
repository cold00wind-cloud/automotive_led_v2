#pragma once
/*
 * Pin — "адреса" ніжки мікроконтролера + її активний рівень.
 *
 * Активний рівень (active) — це рівень, при якому пристрій "увімкнений":
 *   - кнопка KEY на PA0 замикає на землю → active = GPIO_PIN_RESET;
 *   - LED на PC13 світиться, коли пін у нулі → active = GPIO_PIN_RESET.
 * Компоненти вище думають тільки "увімкнено/вимкнено" (true/false), а
 * перетворення в SET/RESET відбувається тут, в одному місці.
 *
 * Pin — це КОНФІГ (const, лежить у flash), а не стан.
 */
#include <stdbool.h>
#include "stm32f4xx_hal.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin; /* GPIO_PIN_x; він же номер лінії EXTI (біт x) */
    GPIO_PinState active;
} Pin;

/* Вихід. Спершу виставляє НЕактивний рівень, потім вмикає вихід — без "смикання" на старті.
 * mode: GPIO_MODE_OUTPUT_PP або GPIO_MODE_OUTPUT_OD. */
void pin_init_output(const Pin *p, uint32_t mode);

/* Вхід з перериванням на обидва фронти. Підтяжка обирається з active:
 * active = RESET → pull-up, active = SET → pull-down. */
void pin_init_irq_input(const Pin *p);

bool pin_is_active(const Pin *p);
void pin_set_active(const Pin *p, bool active);

/* Заборонити / дозволити переривання саме цієї лінії EXTI (не всього NVIC). */
void pin_irq_mask(const Pin *p);
void pin_irq_unmask(const Pin *p);
