#pragma once
/*
 * Подія — єдиний "лист", яким спілкуються частини програми.
 *
 * Хто б не був джерелом (переривання кнопки, таймер, компонент Button),
 * він не викликає отримувача напряму, а кладе подію в чергу. Головний цикл
 * дістає подію і віддає тому, кому вона адресована.
 *
 * Event навмисно маленький (3 байти + вирівнювання): його копіюють у чергу
 * та з черги, тож велика структура = повільніший ISR.
 */
#include <stdint.h>

typedef enum
{
    SIG_PIN_EDGE,   /* DL → CL: на піні кнопки був фронт (ще з брязкотом)   id = ButtonId */
    SIG_TIMEOUT,    /* DL → CL: сплив програмний таймер                      id = TimerId  */
    SIG_BTN_SHORT,  /* CL → BL: коротке натискання (після debounce)          id = ButtonId */
    SIG_BTN_DOUBLE, /* CL → BL: подвійне натискання                          id = ButtonId */
    SIG_BTN_LONG,   /* CL → BL: утримання довше за long_ms                   id = ButtonId */
} Signal;

typedef struct
{
    Signal sig;  /* ЩО сталося */
    uint8_t id;  /* З КИМ сталося (кнопка або таймер, залежить від sig) */
    uint8_t tag; /* Для SIG_TIMEOUT: "покоління" таймера, див. soft_timer.h. Інакше 0 */
} Event;
