#pragma once
/*
 * Lock (CL) — замок на реле. Стейт-машина з двох станів: LOCKED ⇄ UNLOCKED.
 *
 * Компонент знає ЯК відкрити замок (який пін, який рівень), але не знає
 * КОЛИ — це вирішує бізнес-логіка (Car).
 */
#include <stdbool.h>
#include "dl/gpio_pin.h"

typedef enum
{
    LOCK_LOCKED,
    LOCK_UNLOCKED,
} LockState;

typedef struct
{
    const Pin *relay;
    LockState state;
} Lock;

void lock_init(Lock *l, const Pin *relay); /* старт: LOCKED */
void lock_open(Lock *l);
void lock_close(Lock *l);
void lock_toggle(Lock *l);
bool lock_is_open(const Lock *l);
