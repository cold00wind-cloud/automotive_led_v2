#include "cl/lock.h"

/* Один вихід на залізо: стан і пін завжди змінюються разом. */
static void set_state(Lock *l, LockState s)
{
    l->state = s;
    pin_set_active(l->relay, s == LOCK_UNLOCKED);
}

void lock_init(Lock *l, const Pin *relay)
{
    l->relay = relay;
    set_state(l, LOCK_LOCKED);
}

void lock_open(Lock *l) { set_state(l, LOCK_UNLOCKED); }
void lock_close(Lock *l) { set_state(l, LOCK_LOCKED); }

void lock_toggle(Lock *l)
{
    set_state(l, lock_is_open(l) ? LOCK_LOCKED : LOCK_UNLOCKED);
}

bool lock_is_open(const Lock *l)
{
    return l->state == LOCK_UNLOCKED;
}
