/*
 * i18n.c — таблица переводов и определение языка системы.
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "i18n.h"
#include "settings.h"
#include "srt.h"

/* Таблица переводов: [язык][идентификатор].
 * 0 — русский, 1 — английский.
 * Инициализаторы именованные: порядок строк не важен, а забыть строку
 * невозможно — недостающие останутся пустыми. */
static const WCHAR *const TEXTS[2][S_COUNT] = {
/* --- русский --- */
{
    [S_APP_TITLE]          = L"SnowRunner — Управление временем",
    [S_HEADER]             = L"SnowRunner Time Jumper",
    [S_CONNECTED_NO]       = L"Соединение с игрой не установлено",
    [S_MEMORY_CHANGED]     = L"%ls · PID %ls · память игры изменена",
    [S_SECTION_TIME]       = L"УПРАВЛЕНИЕ ВРЕМЕНЕМ",
    [S_SECTION_KEYS]       = L"ГОРЯЧИЕ КЛАВИШИ",

    [S_CHIP_NO_GAME]       = L"Нет игры",
    [S_CHIP_NORMAL]        = L"Обычное",
    [S_CHIP_STOP]          = L"Стоп",
    [S_CHIP_SYNC]          = L"По часам",
    [S_DESC_NO_GAME]       = L"Запустите SnowRunner и нажмите «Подключиться»",
    [S_DESC_NORMAL]        = L"Время идёт с обычной скоростью игры",
    [S_DESC_STOP]          = L"Время остановлено",
    [S_DESC_SYNC]          = L"Время синхронизировано с часами компьютера",
    [S_DESC_SPEED]         = L"Скорость изменения времени",

    [S_BTN_STOP]           = L"Остановить",
    [S_BTN_START]          = L"Запустить",
    [S_BTN_SYNC]           = L"По часам",
    [S_BTN_CONNECT]        = L"Подключиться к игре",
    [S_BTN_DISCONNECT]     = L"Отключиться",
    [S_BTN_EXACT]          = L"Точное время ▾",
    [S_SHIFT_M1]           = L"−1 ч",
    [S_SHIFT_P1]           = L"+1 ч",
    [S_SHIFT_M2]           = L"−2 ч",
    [S_SHIFT_P2]           = L"+2 ч",

    [S_THEME_SYSTEM]       = L"Системная",
    [S_THEME_LIGHT]        = L"Светлая",
    [S_THEME_DARK]         = L"Тёмная",
    [S_LANG_SYSTEM]        = L"Системный",
    [S_LANG_RU]            = L"Русский",
    [S_LANG_EN]            = L"English",

    [S_ST_CONNECTED]       = L"Подключено. Управление временем доступно.",
    [S_ST_DISCONNECTED]    = L"Отключено от игры, память игры восстановлена.",
    [S_ST_GAME_CLOSED]     = L"Игра закрыта — память восстановлена.",
    [S_ST_STOP_OK]         = L"Время остановлено.",
    [S_ST_START_OK]        = L"Время идёт в обычном режиме игры.",
    [S_ST_SYNC_OK]         = L"Время синхронизировано с часами компьютера.",
    [S_ST_SPEED_X]         = L"Скорость времени: x%g.",
    [S_ST_SPEED_DIV]       = L"Скорость времени: /%g.",
    [S_ST_SPEED_NORMAL]    = L"Скорость времени: обычная.",
    [S_ST_SHIFTED]         = L"Время в игре: %02d:%02d. Скорость обычная.",
    [S_ST_TIME_SET]        = L"Время установлено на %02d:%02d. Скорость обычная.",
    [S_ST_NEED_GAME]       = L"Сначала подключитесь к игре.",
    [S_ST_READ_FAIL]       = L"Не удалось прочитать время в игре.",
    [S_ST_SET_FAIL]        = L"Не удалось изменить время.",
    [S_ST_STOP_FAIL]       = L"Не удалось остановить время.",
    [S_ST_START_FAIL]      = L"Не удалось запустить время.",
    [S_ST_SYNC_FAIL]       = L"Не удалось включить синхронизацию.",
    [S_ST_SPEED_FAIL]      = L"Не удалось изменить скорость.",
    [S_ST_THEME_CHANGED]   = L"Тема оформления изменена.",
    [S_ST_LANG_CHANGED]    = L"Язык интерфейса изменён.",
    [S_ST_HOTKEY_TAKEN]    = L"Не удалось зарегистрировать сочетание %ls (оно уже занято другой программой).",

    [S_ERR_NOT_FOUND]      = L"Процесс %ls не найден.\n\nЗапустите игру и нажмите «Подключиться».",
    [S_ERR_OPEN]           = L"Не удалось открыть процесс игры (код %lu).\n\nОбычно это значит, что игра запущена с повышенными правами.\nЗапустите эту программу от имени администратора.",
    [S_ERR_MODULE]         = L"Не удалось найти модуль игры в памяти.",
    [S_ERR_PATTERN]        = L"В модуле игры не найдено инструкции обновления часов.\nПохоже, игра обновилась: напишите разработчику программы.",
    [S_ERR_AMBIGUOUS]      = L"В коде игры найдено несколько похожих мест (%d), а известной сигнатуры нет.\n\nПодключение не выполняется, чтобы не нарушить работу игры. Похоже, игра обновилась: напишите разработчику.",
    [S_ERR_ALLOC]          = L"Не удалось выделить память в процессе игры.",
    [S_ERR_IO]             = L"Не удалось записать код в память игры.",
    [S_ERR_PATCH]          = L"Не удалось изменить код игры.",
    [S_ERR_GFX]            = L"Не удалось инициализировать графическую подсистему GDI+.",
    [S_ERR_GENERIC]        = L"Не удалось подключиться к игре.",

    [S_LOG_DPI]            = L"DPI: SetProcessDpiAwarenessContext -> %d",
    [S_LOG_SCAN_EXACT]     = L"scan: точных совпадений %d",
    [S_LOG_SCAN_ADDR]      = L"scan:   адрес=%llX смещение=0x%X",
    [S_LOG_SCAN_FREE]      = L"scan: свободный поиск дал единственное совпадение",
    [S_LOG_ATTACH_PID]     = L"attach: pid=%lu base=%llX кандидатов=%d",
    [S_LOG_ATTACH_CAND]    = L"attach: кандидат %d/%d адрес=%llX смещение=0x%ls",
    [S_LOG_ATTACH_BLOCK]   = L"attach: блок кода=%llX блок данных=%llX",
    [S_LOG_ATTACH_OK]      = L"attach: OK hook=%llX disp=0x%X code=%llX",
    [S_LOG_ATTACH_RETRY]   = L"attach: кандидат %d не подтверждён, пробуем следующий",
    [S_LOG_PATCH_BYTES]    = L"patch: на месте не MOVSS [r13+disp], xmm2 — перехват отменён",
    [S_LOG_PATCH_MISMATCH] = L"patch: смещение не совпало (найдено 0x%X, ожидалось 0x%X) — перехват отменён",
    [S_LOG_VERIFY]         = L"verify: clock_addr=%llX natural=%f",
    [S_LOG_VERIFY_BAD]     = L"verify: значение %f не похоже на время — перехват отменён",
    [S_LOG_DETACH]         = L"detach: PID %lu",
    [S_LOG_GFX_FAMILY]     = L"шрифт: GdipCreateFontFamilyFromName -> ошибка",
    [S_LOG_GFX_FONT]       = L"шрифт: GdipCreateFont(size=%d bold=%d) -> %d",
    [S_LOG_SNAPSHOT_OK]    = L"снимок окна сохранён: %ls",
    [S_LOG_SNAPSHOT_FAIL]  = L"не удалось сохранить снимок окна: %ls",

    [S_HK_STOP]            = L"Остановить время",
    [S_HK_START]           = L"Запустить время",
    [S_HK_SPEED_1]         = L"Обычная скорость",
    [S_HK_SPEED_2]         = L"Ускорение x2",
    [S_HK_SPEED_3]         = L"Ускорение x3",
    [S_HK_SPEED_4]         = L"Ускорение x4",
    [S_HK_SLOW_2]          = L"Замедление /2",
    [S_HK_SLOW_3]          = L"Замедление /3",
    [S_HK_SLOW_4]          = L"Замедление /4",
    [S_HK_BACK_1]          = L"Время назад на 1 час",
    [S_HK_FWD_1]           = L"Время вперёд на 1 час",
    [S_HK_FWD_2]           = L"Время вперёд на 2 часа",
    [S_HK_BACK_2]          = L"Время назад на 2 часа",
    [S_HK_T_00]            = L"Установить 00:00",
    [S_HK_T_06]            = L"Установить 06:00",
    [S_HK_T_09]            = L"Установить 09:00",
    [S_HK_T_12]            = L"Установить 12:00",
    [S_HK_T_14]            = L"Установить 14:00",
    [S_HK_T_16]            = L"Установить 16:00",
    [S_HK_T_18]            = L"Установить 18:00",
    [S_HK_T_20]            = L"Установить 20:00",
    [S_HK_T_22]            = L"Установить 22:00",
    [S_HK_T_24]            = L"Установить 24:00",
    [S_HK_SYNC]            = L"По часам"
},
/* --- английский --- */
{
    [S_APP_TITLE]          = L"SnowRunner — Time Control",
    [S_HEADER]             = L"SnowRunner Time Jumper",
    [S_CONNECTED_NO]       = L"Not connected to the game",
    [S_MEMORY_CHANGED]     = L"%ls · PID %ls · game memory modified",
    [S_SECTION_TIME]       = L"TIME CONTROL",
    [S_SECTION_KEYS]       = L"HOTKEYS",

    [S_CHIP_NO_GAME]       = L"No game",
    [S_CHIP_NORMAL]        = L"Normal",
    [S_CHIP_STOP]          = L"Stopped",
    [S_CHIP_SYNC]          = L"PC clock",
    [S_DESC_NO_GAME]       = L"Start SnowRunner and press \"Connect to the game\"",
    [S_DESC_NORMAL]        = L"Time runs at the game's normal speed",
    [S_DESC_STOP]          = L"Time is stopped",
    [S_DESC_SYNC]          = L"Time is synced with the computer clock",
    [S_DESC_SPEED]         = L"Time rate",

    [S_BTN_STOP]           = L"Stop",
    [S_BTN_START]          = L"Start",
    [S_BTN_SYNC]           = L"PC clock",
    [S_BTN_CONNECT]        = L"Connect to the game",
    [S_BTN_DISCONNECT]     = L"Disconnect",
    [S_BTN_EXACT]          = L"Exact time ▾",
    [S_SHIFT_M1]           = L"−1 h",
    [S_SHIFT_P1]           = L"+1 h",
    [S_SHIFT_M2]           = L"−2 h",
    [S_SHIFT_P2]           = L"+2 h",

    [S_THEME_SYSTEM]       = L"System",
    [S_THEME_LIGHT]        = L"Light",
    [S_THEME_DARK]         = L"Dark",
    [S_LANG_SYSTEM]        = L"System",
    [S_LANG_RU]            = L"Russian",
    [S_LANG_EN]            = L"English",

    [S_ST_CONNECTED]       = L"Connected. Time control is available.",
    [S_ST_DISCONNECTED]    = L"Disconnected, game memory restored.",
    [S_ST_GAME_CLOSED]     = L"Game closed - memory restored.",
    [S_ST_STOP_OK]         = L"Time stopped.",
    [S_ST_START_OK]        = L"Time runs in the game's normal mode.",
    [S_ST_SYNC_OK]         = L"Time synced with the computer clock.",
    [S_ST_SPEED_X]         = L"Time rate: x%g.",
    [S_ST_SPEED_DIV]       = L"Time rate: /%g.",
    [S_ST_SPEED_NORMAL]    = L"Time rate: normal.",
    [S_ST_SHIFTED]         = L"In-game time: %02d:%02d. Normal rate.",
    [S_ST_TIME_SET]        = L"Time set to %02d:%02d. Normal rate.",
    [S_ST_NEED_GAME]       = L"Connect to the game first.",
    [S_ST_READ_FAIL]       = L"Could not read the in-game time.",
    [S_ST_SET_FAIL]        = L"Could not change the time.",
    [S_ST_STOP_FAIL]       = L"Could not stop the time.",
    [S_ST_START_FAIL]      = L"Could not start the time.",
    [S_ST_SYNC_FAIL]       = L"Could not enable clock sync.",
    [S_ST_SPEED_FAIL]      = L"Could not change the time rate.",
    [S_ST_THEME_CHANGED]   = L"Theme changed.",
    [S_ST_LANG_CHANGED]    = L"Interface language changed.",
    [S_ST_HOTKEY_TAKEN]    = L"Could not register the %ls hotkey (already taken by another program).",

    [S_ERR_NOT_FOUND]      = L"Process %ls not found.\n\nStart the game and press \"Connect to the game\".",
    [S_ERR_OPEN]           = L"Could not open the game process (code %lu).\n\nThis usually means the game runs with elevated rights.\nRun this program as administrator.",
    [S_ERR_MODULE]         = L"Could not find the game module in memory.",
    [S_ERR_PATTERN]        = L"The clock update instruction was not found in the game module.\nThe game has probably been updated: please report it.",
    [S_ERR_AMBIGUOUS]      = L"Found %d similar places in the game code and no known signature.\n\nNot connecting, to avoid breaking the game. The game has probably been updated: please report it.",
    [S_ERR_ALLOC]          = L"Could not allocate memory in the game process.",
    [S_ERR_IO]             = L"Could not write the code into the game memory.",
    [S_ERR_PATCH]          = L"Could not modify the game code.",
    [S_ERR_GFX]            = L"Could not initialize the GDI+ graphics subsystem.",
    [S_ERR_GENERIC]        = L"Could not connect to the game.",

    [S_LOG_DPI]            = L"DPI: SetProcessDpiAwarenessContext -> %d",
    [S_LOG_SCAN_EXACT]     = L"scan: exact matches %d",
    [S_LOG_SCAN_ADDR]      = L"scan:   address=%llX offset=0x%X",
    [S_LOG_SCAN_FREE]      = L"scan: free search returned a single match",
    [S_LOG_ATTACH_PID]     = L"attach: pid=%lu base=%llX candidates=%d",
    [S_LOG_ATTACH_CAND]    = L"attach: candidate %d/%d address=%llX offset=0x%ls",
    [S_LOG_ATTACH_BLOCK]   = L"attach: code block=%llX data block=%llX",
    [S_LOG_ATTACH_OK]      = L"attach: OK hook=%llX disp=0x%X code=%llX",
    [S_LOG_ATTACH_RETRY]   = L"attach: candidate %d not confirmed, trying the next one",
    [S_LOG_PATCH_BYTES]    = L"patch: not a MOVSS [r13+disp], xmm2 there - hook cancelled",
    [S_LOG_PATCH_MISMATCH] = L"patch: offset mismatch (found 0x%X, expected 0x%X) - hook cancelled",
    [S_LOG_VERIFY]         = L"verify: clock_addr=%llX natural=%f",
    [S_LOG_VERIFY_BAD]     = L"verify: value %f does not look like the clock - hook cancelled",
    [S_LOG_DETACH]         = L"detach: PID %lu",
    [S_LOG_GFX_FAMILY]     = L"font: GdipCreateFontFamilyFromName -> error",
    [S_LOG_GFX_FONT]       = L"font: GdipCreateFont(size=%d bold=%d) -> %d",
    [S_LOG_SNAPSHOT_OK]    = L"window snapshot saved: %ls",
    [S_LOG_SNAPSHOT_FAIL]  = L"could not save the window snapshot: %ls",

    [S_HK_STOP]            = L"Stop the clock",
    [S_HK_START]           = L"Start the clock",
    [S_HK_SPEED_1]         = L"Normal speed",
    [S_HK_SPEED_2]         = L"Speed up x2",
    [S_HK_SPEED_3]         = L"Speed up x3",
    [S_HK_SPEED_4]         = L"Speed up x4",
    [S_HK_SLOW_2]          = L"Slow down /2",
    [S_HK_SLOW_3]          = L"Slow down /3",
    [S_HK_SLOW_4]          = L"Slow down /4",
    [S_HK_BACK_1]          = L"One hour back",
    [S_HK_FWD_1]           = L"One hour forward",
    [S_HK_FWD_2]           = L"Two hours forward",
    [S_HK_BACK_2]          = L"Two hours back",
    [S_HK_T_00]            = L"Set 00:00",
    [S_HK_T_06]            = L"Set 06:00",
    [S_HK_T_09]            = L"Set 09:00",
    [S_HK_T_12]            = L"Set 12:00",
    [S_HK_T_14]            = L"Set 14:00",
    [S_HK_T_16]            = L"Set 16:00",
    [S_HK_T_18]            = L"Set 18:00",
    [S_HK_T_20]            = L"Set 20:00",
    [S_HK_T_22]            = L"Set 22:00",
    [S_HK_T_24]            = L"Set 24:00",
    [S_HK_SYNC]            = L"PC clock"
}
};

static LangMode g_mode = LANG_SYSTEM;

/* ------------------------------------------------------------------ */

BOOL lang_system_is_russian(void)
{
    wchar_t name[64] = { 0 };

    /* Язык пользователя (региональные параметры). */
    if (GetUserDefaultLocaleName(name, 64) > 0) {
        if ((name[0] == 'r' || name[0] == 'R') &&
            (name[1] == 'u' || name[1] == 'U'))
            return TRUE;
        name[0] = 0;
    }
    /* Язык интерфейса Windows. */
    if (GetSystemDefaultUILanguage() == 0x0419)   /* русский */
        return TRUE;
    return FALSE;
}

int lang_index(void)
{
    switch (g_mode) {
    case LANG_RU: return 0;
    case LANG_EN: return 1;
    default:      return lang_system_is_russian() ? 0 : 1;
    }
}

void lang_set_mode(LangMode mode)
{
    g_mode = mode;
}

LangMode lang_mode(void)
{
    return g_mode;
}

const WCHAR *L(StrId id)
{
    if ((int)id < 0 || id >= S_COUNT) return L"";
    return TEXTS[lang_index()][id];
}

StrId lang_label(void)
{
    switch (g_mode) {
    case LANG_RU: return S_LANG_RU;
    case LANG_EN: return S_LANG_EN;
    default:      return S_LANG_SYSTEM;
    }
}

void lang_format(WCHAR *buf, int chars, StrId id, ...)
{
    if (!buf || chars <= 0) return;
    const WCHAR *fmt = L(id);
    va_list ap;

    memset(buf, 0, (size_t)chars * sizeof(WCHAR));
    va_start(ap, id);
    _vsnwprintf(buf, chars - 1, fmt, ap);
    va_end(ap);
    buf[chars - 1] = 0;
}

void lang_load(void)
{
    char value[64] = { 0 };
    if (!settings_get("lang", value, sizeof(value))) {
        srt_log("i18n: no 'lang' key, using the system language");
        return;
    }
    if (strcmp(value, "ru") == 0)      g_mode = LANG_RU;
    else if (strcmp(value, "en") == 0) g_mode = LANG_EN;
    else                               g_mode = LANG_SYSTEM;
    srt_log("i18n: lang from settings = %s, index = %d", value, lang_index());
}

void lang_save(void)
{
    const char *v = "system";
    if (g_mode == LANG_RU) v = "ru";
    else if (g_mode == LANG_EN) v = "en";
    settings_set("lang", v);
}

void log_msg(StrId id, ...)
{
    const WCHAR *fmt = L(id);
    wchar_t buf[512];
    va_list ap;

    memset(buf, 0, sizeof(buf));
    va_start(ap, id);
    _vsnwprintf(buf, (int)(sizeof(buf) / sizeof(buf[0])) - 1, fmt, ap);
    va_end(ap);
    buf[(int)(sizeof(buf) / sizeof(buf[0])) - 1] = 0;

    /* В журнал пишем UTF-8. */
    char utf8[1024];
    int n = WideCharToMultiByte(CP_UTF8, 0, buf, -1, utf8, (int)sizeof(utf8), NULL, NULL);
    if (n > 0) srt_log("%s", utf8);
}
