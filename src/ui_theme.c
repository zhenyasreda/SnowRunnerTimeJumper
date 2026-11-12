/*
 * ui_theme.c — оформление интерфейса и его сохранение.
 *
 * Настройка хранится в файле %LOCALAPPDATA%\SnowRunnerTimeJumper\settings.ini
 * (реестр не используется).
 */
#include <stdio.h>
#include <string.h>
#include "ui_theme.h"
#include "srt.h"

static const UiPalette PAL_DARK = {
    RGB(22, 24, 31),        /* bg         */
    RGB(31, 34, 43),        /* card       */
    RGB(46, 50, 63),        /* card_edge  */
    RGB(233, 236, 243),     /* text       */
    RGB(139, 146, 166),     /* dim        */
    RGB(96, 102, 120),      /* faint      */
    RGB(88, 142, 255),      /* accent     */
    RGB(116, 164, 255),     /* accent_hi  */
    RGB(76, 200, 130),      /* ok         */
    RGB(240, 182, 74),      /* warn       */
    RGB(233, 96, 96),       /* danger     */
    RGB(42, 46, 58),        /* btn        */
    RGB(55, 60, 76),        /* btn_hi     */
    RGB(226, 229, 240),     /* btn_txt    */
    RGB(20, 22, 28)         /* chip_txt   */
};

static const UiPalette PAL_LIGHT = {
    RGB(245, 246, 249),     /* bg         */
    RGB(255, 255, 255),     /* card       */
    RGB(223, 226, 233),     /* card_edge  */
    RGB(28, 31, 38),        /* text       */
    RGB(96, 102, 118),      /* dim        */
    RGB(140, 146, 160),     /* faint      */
    RGB(42, 105, 220),      /* accent     */
    RGB(66, 124, 232),      /* accent_hi  */
    RGB(24, 150, 90),       /* ok         */
    RGB(197, 132, 10),      /* warn       */
    RGB(210, 60, 60),       /* danger     */
    RGB(233, 236, 242),     /* btn        */
    RGB(217, 223, 234),     /* btn_hi     */
    RGB(40, 44, 54),        /* btn_txt    */
    RGB(255, 255, 255)      /* chip_txt   */
};

static UiThemeMode g_mode = UI_THEME_SYSTEM;
static BOOL         g_system_dark = TRUE;

/* Тёмная ли тема в Windows? (HKCU\...\Themes\Personalize\AppsUseLightTheme) */
BOOL ui_theme_system_is_dark(void)
{
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return TRUE;                     /* по умолчанию — тёмная */
    DWORD value = 1, type = 0, size = sizeof(value);
    LONG r = RegQueryValueExW(key, L"AppsUseLightTheme", NULL, &type,
                              (LPBYTE)&value, &size);
    RegCloseKey(key);
    if (r != ERROR_SUCCESS || type != REG_DWORD) return TRUE;
    return value == 0;                   /* 0 — тёмная, 1 — светлая */
}

void ui_theme_set(UiThemeMode mode)
{
    g_mode = mode;
    g_system_dark = ui_theme_system_is_dark();
}

UiThemeMode ui_theme_mode(void)
{
    return g_mode;
}

const UiPalette *ui_theme_palette(void)
{
    BOOL dark = (g_mode == UI_THEME_DARK) ||
                (g_mode == UI_THEME_SYSTEM && g_system_dark);
    return dark ? &PAL_DARK : &PAL_LIGHT;
}

const WCHAR *ui_theme_name(void)
{
    switch (g_mode) {
    case UI_THEME_LIGHT: return L"Светлая";
    case UI_THEME_DARK:  return L"Тёмная";
    default:            return L"Системная";
    }
}

/* Путь к файлу настроек. */
static void settings_path(wchar_t *buf, size_t chars)
{
    wchar_t local[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", local, MAX_PATH);
    if (n == 0 || n > MAX_PATH) wcscpy(local, L".");
    _snwprintf(buf, chars, L"%s\\SnowRunnerTimeJumper", local);
    CreateDirectoryW(buf, NULL);
    _snwprintf(buf, chars, L"%s\\SnowRunnerTimeJumper\\settings.ini", local);
}

void ui_theme_load(void)
{
    wchar_t path[MAX_PATH];
    settings_path(path, MAX_PATH);

    FILE *f = NULL;
    if (_wfopen_s(&f, path, L"r") != 0 || !f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "theme=", 6) != 0) continue;
        const char *v = line + 6;
        if (strncmp(v, "light", 5) == 0)      g_mode = UI_THEME_LIGHT;
        else if (strncmp(v, "dark", 4) == 0) g_mode = UI_THEME_DARK;
        else                                 g_mode = UI_THEME_SYSTEM;
        break;
    }
    fclose(f);
    g_system_dark = ui_theme_system_is_dark();
}

void ui_theme_save(void)
{
    wchar_t path[MAX_PATH];
    settings_path(path, MAX_PATH);

    FILE *f = NULL;
    if (_wfopen_s(&f, path, L"w") != 0 || !f) return;

    const char *v = "system";
    if (g_mode == UI_THEME_LIGHT) v = "light";
    else if (g_mode == UI_THEME_DARK) v = "dark";
    fprintf(f, "theme=%s\n", v);
    fclose(f);
}
