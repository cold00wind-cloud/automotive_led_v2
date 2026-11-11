#pragma once
/*
 * Car (BL) — бізнес-логіка: ЩО робить машина у відповідь на жести кнопок.
 *
 * BL не знає про піни, таймери debounce чи переривання. Вхід — "кнопка X
 * зробила жест Y", вихід — команди компонентам CL (lock_open, ...).
 */
#include "app_config.h"
#include "cl/lock.h"
#include "dl/event.h"

typedef struct
{
    Lock *lock;
} Car;

void car_init(Car *c, Lock *lock);
void car_on_button(Car *c, ButtonId button, Signal gesture);
