/*
 * hotkeys.c — таблица горячих клавиш.
 *
 * Все сочетания построены на Ctrl+Alt: они не пересекаются ни с игровыми
 * клавишами (в SnowRunner 1—5 переключают вид камеры, Ctrl и Shift
 * отвечают за передачи), ни с привычными сочетаниями браузеров (Ctrl+1…9,
 * Ctrl+«+»/«-»).
 */
#include "hotkeys.h"

#define MODS (MOD_CONTROL | MOD_ALT)

const HotkeyDef g_hotkeys[] = {
    /* --- пуск/стоп --- */
    { 1, ACT_STOP,  MODS, 'S',  "Ctrl+Alt+S", S_HK_STOP,   0.0f },
    { 2, ACT_START, MODS, 'P',  "Ctrl+Alt+P", S_HK_START,  0.0f },

    /* --- скорость --- */
    { 3, ACT_SPEED, MODS, '1',  "Ctrl+Alt+1", S_HK_SPEED_1, 1.00f },
    { 4, ACT_SPEED, MODS, '2',  "Ctrl+Alt+2", S_HK_SPEED_2, 2.00f },
    { 5, ACT_SPEED, MODS, '3',  "Ctrl+Alt+3", S_HK_SPEED_3, 3.00f },
    { 6, ACT_SPEED, MODS, '4',  "Ctrl+Alt+4", S_HK_SPEED_4, 4.00f },
    { 7, ACT_SPEED, MODS, '5',  "Ctrl+Alt+5", S_HK_SLOW_2,  0.50f },
    { 8, ACT_SPEED, MODS, '6',  "Ctrl+Alt+6", S_HK_SLOW_3,  0.33334f },
    { 9, ACT_SPEED, MODS, '7',  "Ctrl+Alt+7", S_HK_SLOW_4,  0.25f },

    /* --- сдвиг часов --- */
    { 10, ACT_SHIFT, MODS, VK_LEFT,  "Ctrl+Alt+Left",  S_HK_BACK_1, -1.0f },
    { 11, ACT_SHIFT, MODS, VK_RIGHT, "Ctrl+Alt+Right", S_HK_FWD_1,   1.0f },
    { 12, ACT_SHIFT, MODS, VK_UP,    "Ctrl+Alt+Up",    S_HK_FWD_2,   2.0f },
    { 13, ACT_SHIFT, MODS, VK_DOWN,  "Ctrl+Alt+Down",  S_HK_BACK_2, -2.0f },

    /* --- точное время (время продолжает идти) --- */
    { 14, ACT_SET_TIME, MODS, VK_F1,  "Ctrl+Alt+F1",  S_HK_T_00,  0.0f },
    { 15, ACT_SET_TIME, MODS, VK_F2,  "Ctrl+Alt+F2",  S_HK_T_06,  6.0f },
    { 16, ACT_SET_TIME, MODS, VK_F3,  "Ctrl+Alt+F3",  S_HK_T_09,  9.0f },
    { 17, ACT_SET_TIME, MODS, VK_F4,  "Ctrl+Alt+F4",  S_HK_T_12, 12.0f },
    { 18, ACT_SET_TIME, MODS, VK_F5,  "Ctrl+Alt+F5",  S_HK_T_14, 14.0f },
    { 19, ACT_SET_TIME, MODS, VK_F6,  "Ctrl+Alt+F6",  S_HK_T_16, 16.0f },
    { 20, ACT_SET_TIME, MODS, VK_F7,  "Ctrl+Alt+F7",  S_HK_T_18, 18.0f },
    { 21, ACT_SET_TIME, MODS, VK_F8,  "Ctrl+Alt+F8",  S_HK_T_20, 20.0f },
    { 22, ACT_SET_TIME, MODS, VK_F9,  "Ctrl+Alt+F9",  S_HK_T_22, 22.0f },
    { 23, ACT_SET_TIME, MODS, VK_F10, "Ctrl+Alt+F10", S_HK_T_24, 24.0f },

    /* --- синхронизация с реальными часами --- */
    { 24, ACT_SYNC, MODS, 'C', "Ctrl+Alt+C", S_HK_SYNC, 0.0f }
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
