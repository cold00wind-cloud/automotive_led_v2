#pragma once
/*
 * Board — "монтажна схема": на якій ніжці що висить.
 *
 * Це єдиний файл, який треба змінити, якщо ти перепаяв кнопку на інший пін
 * або LED виявився активним нулем. Решта коду працює з const Pin * і не знає
 * ні портів, ні номерів.
 */
#include "dl/gpio_pin.h"

extern const Pin PIN_BTN_LOCK;
extern const Pin PIN_BTN_EMRG;
extern const Pin PIN_BTN_CRASH;

extern const Pin PIN_LED_LEFT;
extern const Pin PIN_LED_RIGHT;
extern const Pin PIN_LED_STOP;
extern const Pin PIN_LED_VALET;

extern const Pin PIN_RELAY;

/* Тактування портів, налаштування пінів, дозвіл переривань EXTI. */
void board_init(void);
