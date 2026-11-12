/*
 * hotkeys.c — таблица горячих клавиш.
 *
 * Все сочетания построены на Ctrl+Alt: они не пересекаются ни с игровыми
 * клавишами (в SnowRunner 1—5 переключают вид камеры, Ctrl и Shift
 * отвечают за передачи), ни с привычными сочетаниями браузеров
 * (Ctrl+1…9, Ctrl+«+»/«-»).
 */
#include "hotkeys.h"

#define MODS (MOD_CONTROL | MOD_ALT)

const HotkeyDef g_hotkeys[] = {
    /* --- пуск/стоп --- */
    { 1, ACT_STOP,  MODS, 'S',  "Ctrl+Alt+S", "Остановить время",                 0.0f },
    { 2, ACT_START, MODS, 'P',  "Ctrl+Alt+P", "Запустить время",                   0.0f },

    /* --- скорость --- */
    { 3, ACT_SPEED, MODS, '1',  "Ctrl+Alt+1", "Обычная скорость",  1.00f },
    { 4, ACT_SPEED, MODS, '2',  "Ctrl+Alt+2", "Ускорение x2",      2.00f },
    { 5, ACT_SPEED, MODS, '3',  "Ctrl+Alt+3", "Ускорение x3",      3.00f },
    { 6, ACT_SPEED, MODS, '4',  "Ctrl+Alt+4", "Ускорение x4",      4.00f },
    { 7, ACT_SPEED, MODS, '5',  "Ctrl+Alt+5", "Замедление /2",     0.50f },
    { 8, ACT_SPEED, MODS, '6',  "Ctrl+Alt+6", "Замедление /3",     0.33334f },
    { 9, ACT_SPEED, MODS, '7',  "Ctrl+Alt+7", "Замедление /4",     0.25f },

    /* --- сдвиг часов --- */
    { 10, ACT_SHIFT, MODS, VK_LEFT,  "Ctrl+Alt+Left",  "Время назад на 1 час",  -1.0f },
    { 11, ACT_SHIFT, MODS, VK_RIGHT, "Ctrl+Alt+Right", "Время вперёд на 1 час",  1.0f },
    { 12, ACT_SHIFT, MODS, VK_UP,    "Ctrl+Alt+Up",    "Время вперёд на 2 часа",  2.0f },
    { 13, ACT_SHIFT, MODS, VK_DOWN,  "Ctrl+Alt+Down",  "Время назад на 2 часа", -2.0f },

    /* --- точное время (время продолжает идти) --- */
    { 14, ACT_SET_TIME, MODS, VK_F1,  "Ctrl+Alt+F1",  "Установить 00:00",  0.0f },
    { 15, ACT_SET_TIME, MODS, VK_F2,  "Ctrl+Alt+F2",  "Установить 06:00",  6.0f },
    { 16, ACT_SET_TIME, MODS, VK_F3,  "Ctrl+Alt+F3",  "Установить 09:00",  9.0f },
    { 17, ACT_SET_TIME, MODS, VK_F4,  "Ctrl+Alt+F4",  "Установить 12:00", 12.0f },
    { 18, ACT_SET_TIME, MODS, VK_F5,  "Ctrl+Alt+F5",  "Установить 14:00", 14.0f },
    { 19, ACT_SET_TIME, MODS, VK_F6,  "Ctrl+Alt+F6",  "Установить 16:00", 16.0f },
    { 20, ACT_SET_TIME, MODS, VK_F7,  "Ctrl+Alt+F7",  "Установить 18:00", 18.0f },
    { 21, ACT_SET_TIME, MODS, VK_F8,  "Ctrl+Alt+F8",  "Установить 20:00", 20.0f },
    { 22, ACT_SET_TIME, MODS, VK_F9,  "Ctrl+Alt+F9",  "Установить 22:00", 22.0f },
    { 23, ACT_SET_TIME, MODS, VK_F10, "Ctrl+Alt+F10", "Установить 24:00", 24.0f },

    /* --- синхронизация с реальными часами --- */
    { 24, ACT_SYNC, MODS, 'C', "Ctrl+Alt+C", "По часам", 0.0f }
};

const int g_hotkey_count = (int)(sizeof(g_hotkeys) / sizeof(g_hotkeys[0]));

const HotkeyDef *hotkey_by_id(int id)
{
    for (int i = 0; i < g_hotkey_count; i++)
        if (g_hotkeys[i].id == id) return &g_hotkeys[i];
    return NULL;
}

const char *hotkey_label(const HotkeyDef *hk)
{
    return hk ? hk->keys : "";
}

const char *hotkey_action_text(const HotkeyDef *hk)
{
    return hk ? hk->desc : "";
}
