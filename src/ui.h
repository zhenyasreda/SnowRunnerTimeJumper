/*
 * ui.h — общее состояние графической части.
 */
#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

#include <windows.h>
#include "srt.h"
#include "hotkeys.h"
#include "ui_gfx.h"

/* Действия кнопок. */
typedef enum {
    UI_ACT_STOP = 1,
    UI_ACT_START,
    UI_ACT_SYNC,
    UI_ACT_SPEED,       /* value — множитель скорости */
    UI_ACT_SHIFT,       /* value — сдвиг часов       */
    UI_ACT_PRESET_MENU, /* меню точного времени      */
    UI_ACT_ATTACH,
    UI_ACT_DETACH,
    UI_ACT_THEME,       /* переключить тему оформления */
    UI_ACT_LANG        /* переключить язык интерфейса */
} UiAction;

typedef enum {
    UI_BTN_NORMAL = 0,
    UI_BTN_ACCENT,
    UI_BTN_DANGER
} UiButtonStyle;

typedef struct {
    RECT      rect;
    UiAction  action;
    float     value;
    WCHAR     label[32];
    UiButtonStyle style;
    int       id;
} UiButton;

typedef struct {
    HWND        hwnd;
    SrtSession *session;

    int   dpi;
    int   scale;          /* масштаб в процентах от 96 dpi */

    /* текущее состояние */
    float clock_hours;
    BOOL  clock_valid;
    int   mode;           /* SrtMode */
    int   rate_kind;      /* 0 — нет, 1 — скорость, 2 — по часам */
    double rate_ms;

    WCHAR status[512];
    int   status_kind;    /* 0 — норма, 1 — внимание, 2 — ошибка */
    int   demo;           /* режим макета интерфейса (--demo) */
    int   read_fail;      /* сколько раз подряд не удалось прочитать часы */

    /* элементы интерфейса */
    UiButton buttons[24];
    int      button_count;
    int      hover_id;

    /* Области, которые перерисовываются по изменению состояния. */
    RECT rect_clock;      /* часы и подпись режима */
    RECT rect_chip;       /* чип с режимом        */
    RECT rect_status;     /* строка состояния    */
    WCHAR shown_clock[32];
    int   shown_mode;     /* кэш состояния для проверки изменений */

    GfxFont *f_title;
    GfxFont *f_sub;
    GfxFont *f_clock;
    GfxFont *f_chip;
    GfxFont *f_btn;
    GfxFont *f_sect;
    GfxFont *f_key;
    GfxFont *f_desc;
    GfxFont *f_status;
} UiState;

/* Пересчитать геометрию элементов (вызывается при изменении размера окна). */
void ui_layout(UiState *st, int width, int height);

/* Отрисовать окно (с двойной буферизацией — без мерцания). */
void ui_paint(UiState *st, HDC hdc, int width, int height);

/* Отформатировать часы в виде "ЧЧ:ММ" (строка всегда завершена нулём). */
void ui_format_clock(float hours, WCHAR *out, int n);

/* Создать/удалить шрифты. */
BOOL ui_fonts_create(UiState *st);
void ui_fonts_destroy(UiState *st);

#endif /* UI_H_INCLUDED */
