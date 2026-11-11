#pragma once
/*
 * Конфіг застосунку: усі ідентифікатори та всі часові константи в ОДНОМУ місці.
 *
 * Чому id живуть тут, а не в DL: черга й таймери (DL) не повинні знати, що
 * в нас є "кнопка замка" чи "лівий поворотник" — для них це просто число
 * uint8_t. Тоді DL можна взяти в інший проєкт без змін.
 */

/* ---------- Час (вимоги завдання) ---------- */
#define DEBOUNCE_MS     50u   /* брязкіт контактів затихає за ~5–20 мс, беремо із запасом */
#define LONG_PRESS_MS   2000u /* вимога 5 */
#define DOUBLE_CLICK_MS 300u  /* вікно між відпусканням і другим натисканням */
#define BLINK_MS        500u  /* вимога 4: 500 мс горить, 500 мс не горить */
#define BRAKE_MS        1000u /* скільки горить STOP після короткого удару */

/* ---------- Кнопки ---------- */
typedef enum
{
    BTN_LOCK,  /* PA0: замок / valet */
    BTN_EMRG,  /* PB4: поворотники (коротке/подвійне) + аварійка (довге) */
    BTN_CRASH, /* PB1: датчик удару */
    BTN_COUNT
} ButtonId;

/* ---------- Світлодіоди ---------- */
typedef enum
{
    LED_LEFT,
    LED_RIGHT,
    LED_STOP,
    LED_VALET,
    LED_COUNT
} LedId;

/* ---------- Програмні таймери ----------
 * Кожна кнопка має 2 таймери (debounce і жест), кожен LED — 1.
 * Номери йдуть блоками, щоб з номера таймера одразу знати власника:
 *   [0 .. BTN_COUNT)              — debounce кнопки №(id)
 *   [BTN_COUNT .. 2*BTN_COUNT)    — жест кнопки №(id - BTN_COUNT)
 *   [2*BTN_COUNT .. +LED_COUNT)   — LED №(id - 2*BTN_COUNT)
 */
enum
{
    TMR_DEBOUNCE_BASE = 0,
    TMR_GESTURE_BASE = TMR_DEBOUNCE_BASE + BTN_COUNT,
    TMR_LED_BASE = TMR_GESTURE_BASE + BTN_COUNT,
    TMR_COUNT = TMR_LED_BASE + LED_COUNT
};
