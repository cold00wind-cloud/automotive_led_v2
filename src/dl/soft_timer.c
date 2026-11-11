#include "dl/soft_timer.h"
#include "dl/critical.h"
#include "dl/event_queue.h"

typedef struct
{
    uint32_t remaining; /* 0 = таймер не працює */
    uint32_t period;    /* 0 = одноразовий */
    uint8_t generation; /* див. "покоління" у soft_timer.h */
} SoftTimer;

static SoftTimer timers[ST_MAX_TIMERS];

void st_start(uint8_t id, uint32_t ms, bool periodic)
{
    if (id >= ST_MAX_TIMERS || ms == 0u)
    {
        return;
    }
    uint32_t primask = critical_enter();
    timers[id].remaining = ms;
    timers[id].period = periodic ? ms : 0u;
    timers[id].generation++;
    critical_exit(primask);
}

void st_stop(uint8_t id)
{
    if (id >= ST_MAX_TIMERS)
    {
        return;
    }
    uint32_t primask = critical_enter();
    timers[id].remaining = 0u;
    timers[id].generation++;
    critical_exit(primask);
}

bool st_is_current(uint8_t id, uint8_t tag)
{
    /* generation змінює тільки головний цикл (st_start/st_stop),
     * і цю функцію теж викликає головний цикл — секція не потрібна. */
    return id < ST_MAX_TIMERS && timers[id].generation == tag;
}

void st_tick(void)
{
    /* Ми вже всередині ISR SysTick: головний цикл нас не перерве. */
    for (uint8_t id = 0; id < ST_MAX_TIMERS; id++)
    {
        SoftTimer *t = &timers[id];
        if (t->remaining == 0u)
        {
            continue;
        }
        t->remaining--;
        if (t->remaining == 0u)
        {
            eq_post((Event){.sig = SIG_TIMEOUT, .id = id, .tag = t->generation});
            t->remaining = t->period; /* періодичний — знову рахує, одноразовий — стоїть */
        }
    }
}
