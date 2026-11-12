#pragma once
/*
 * Логіка машини: що робити у відповідь на жести кнопок.
 * Тут живуть поворотники, STOP, LED valet і реле замка.
 */
#include "button.h"

typedef enum
{
    BTN_LOCK,  /* PA0: замок / valet */
    BTN_EMRG,  /* PB4: поворотники (коротке / подвійне) + аварійка (довге) */
    BTN_CRASH, /* PB1: датчик удару */
    BTN_COUNT
} ButtonId;

void car_init(void);
void car_on_button(ButtonId button, ButtonEvent event);

/* Викликати в кожному проході головного циклу (блимання, згасання STOP). */
void car_update(void);
