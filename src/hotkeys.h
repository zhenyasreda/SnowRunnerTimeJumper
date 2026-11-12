/*
 * hotkeys.h — единая таблица горячих клавиш.
 *
 * Таблица используется и для регистрации сочетаний в системе, и для
 * отрисовки подсказки в окне программы — они не могут разойтись.
 */
#ifndef HOTKEYS_H_INCLUDED
#define HOTKEYS_H_INCLUDED

#include <windows.h>

typedef enum {
    ACT_STOP = 0,     /* остановить время                      */
    ACT_START,        /* запустить время (обычная скорость)      */
    ACT_SPEED,        /* изменить скорость: value = множитель    */
    ACT_SHIFT,        /* сдвинуть часы: value = часы (+/-)       */
    ACT_SET_TIME,     /* установить точное время: value = часы   */
    ACT_SYNC          /* синхронизировать с реальными часами      */
} HotkeyAction;

typedef struct {
    int          id;          /* идентификатор WM_HOTKEY           */
    HotkeyAction action;
    UINT         mods;        /* MOD_CONTROL | MOD_ALT            */
    UINT         vk;          /* виртуальная клавиша               */
    const char  *keys;        /* "Ctrl+Alt+S"                      */
    const char  *desc;        /* "Остановить время"                */
    float        value;       /* параметр действия                 */
} HotkeyDef;

extern const HotkeyDef g_hotkeys[];
extern const int        g_hotkey_count;

/* Найти описание по идентификатору (NULL, если не найдено). */
const HotkeyDef *hotkey_by_id(int id);

/* Подсказка в виде строки, например "Ctrl+Alt+S" -> "Ctrl+Alt+S". */
const char *hotkey_label(const HotkeyDef *hk);

/* Человекочитаемое описание действия. */
const char *hotkey_action_text(const HotkeyDef *hk);

#endif /* HOTKEYS_H_INCLUDED */
