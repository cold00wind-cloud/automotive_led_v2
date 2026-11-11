#include "bl/car.h"

void car_init(Car *c, Lock *lock)
{
    c->lock = lock;
}

void car_on_button(Car *c, ButtonId button, Signal gesture)
{
    if (button == BTN_LOCK && gesture == SIG_BTN_SHORT)
    {
        lock_toggle(c->lock);
    }
}
