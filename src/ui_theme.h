/*
 * ui_theme.h — оформление интерфейса (светлая / тёмная / системная).
 */
#ifndef UI_THEME_H_INCLUDED
#define UI_THEME_H_INCLUDED

#include <windows.h>

typedef enum {
    UI_THEME_SYSTEM = 0,   /* как в Windows */
    UI_THEME_LIGHT  = 1,
    UI_THEME_DARK   = 2
} UiThemeMode;

/* Палитра интерфейса. */
typedef struct {
    COLORREF bg;
    COLORREF card;
    COLORREF card_edge;
    COLORREF text;
    COLORREF dim;
    COLORREF faint;
    COLORREF accent;
    COLORREF accent_hi;
    COLORREF ok;
    COLORREF warn;
    COLORREF danger;
    COLORREF btn;
    COLORREF btn_hi;
    COLORREF btn_txt;
    COLORREF chip_txt;
} UiPalette;

/* Текущий режим и разрешённая палитра. */
void           ui_theme_set(UiThemeMode mode);
UiThemeMode    ui_theme_mode(void);
const UiPalette *ui_theme_palette(void);

/* Тёмная ли сейчас системная тема Windows? */
BOOL ui_theme_system_is_dark(void);

/* Название режима для интерфейса: «Системная», «Светлая», «Тёмная». */
const WCHAR *ui_theme_name(void);

/* Прочитать сохранённую настройку из файла. */
void ui_theme_load(void);

/* Сохранить настройку в файл. */
void ui_theme_save(void);

#endif /* UI_THEME_H_INCLUDED */
