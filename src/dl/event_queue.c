#include "dl/event_queue.h"
#include "dl/critical.h"

static Event buffer[EQ_CAPACITY];
static uint8_t head;  /* куди писати наступну */
static uint8_t tail;  /* звідки читати наступну */
static uint8_t count; /* скільки зараз лежить */
static uint32_t overflows;

bool eq_post(Event e)
{
    uint32_t primask = critical_enter();
    bool ok = count < EQ_CAPACITY;
    if (ok)
    {
        buffer[head] = e;
        head = (uint8_t)((head + 1u) % EQ_CAPACITY);
        count++;
    }
    else
    {
        overflows++;
    }
    critical_exit(primask);
    return ok;
}

bool eq_pop(Event *out)
{
    uint32_t primask = critical_enter();
    bool ok = count > 0u;
    if (ok)
    {
        *out = buffer[tail];
        tail = (uint8_t)((tail + 1u) % EQ_CAPACITY);
        count--;
    }
    critical_exit(primask);
    return ok;
}

bool eq_is_empty(void)
{
    /* Читання одного байта атомарне. Викликається з вимкненими перериваннями
     * (див. sleep_until_event у main.c), тож значення не зміниться "під ногами". */
    return count == 0u;
}

uint32_t eq_overflow_count(void)
{
    return overflows;
}
