/*
 * settings.c — чтение и запись файла настроек.
 */
#include <stdio.h>
#include <string.h>
#include "settings.h"

#define SETTINGS_DIR  L"SnowRunnerTimeJumper"
#define SETTINGS_FILE L"settings.ini"

static void settings_path(wchar_t *buf, int chars)
{
    wchar_t local[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", local, MAX_PATH);
    if (n == 0 || n > MAX_PATH) wcscpy(local, L".");
    _snwprintf(buf, chars, L"%s\\" SETTINGS_DIR, local);
    CreateDirectoryW(buf, NULL);
    _snwprintf(buf, chars, L"%s\\" SETTINGS_DIR L"\\" SETTINGS_FILE, local);
}

BOOL settings_get(const char *key, char *out, int out_size)
{
    if (!key || !out || out_size <= 0) return FALSE;
    out[0] = 0;

    wchar_t path[MAX_PATH];
    settings_path(path, MAX_PATH);

    FILE *f = NULL;
    if (_wfopen_s(&f, path, L"r") != 0 || !f) return FALSE;

    size_t klen = strlen(key);
    char line[512];
    BOOL found = FALSE;
    while (fgets(line, (int)sizeof(line), f)) {
        if (strncmp(line, key, klen) != 0 || line[klen] != '=') continue;
        const char *v = line + klen + 1;
        size_t n = strlen(v);
        while (n && (v[n - 1] == '\r' || v[n - 1] == '\n')) n--;
        if ((int)n >= out_size) n = (size_t)out_size - 1;
        memcpy(out, v, n);
        out[n] = 0;
        found = TRUE;
        break;
    }
    fclose(f);
    return found;
}

BOOL settings_set(const char *key, const char *value)
{
    if (!key || !value) return FALSE;

    /* Считываем текущее содержимое, чтобы не потерять другие ключи. */
    char keys[32][64];
    char vals[32][128];
    int count = 0;

    wchar_t path[MAX_PATH];
    settings_path(path, MAX_PATH);

    FILE *f = NULL;
    if (_wfopen_s(&f, path, L"r") == 0 && f) {
        char line[512];
        while (fgets(line, (int)sizeof(line), f) && count < 32) {
            char *eq = strchr(line, '=');
            if (!eq) continue;
            size_t klen = (size_t)(eq - line);
            while (klen && (line[klen - 1] == '\r' || line[klen - 1] == '\n' || line[klen - 1] == ' ')) klen--;
            if (klen == 0 || klen >= sizeof(keys[0])) continue;
            memcpy(keys[count], line, klen);
            keys[count][klen] = 0;
            const char *v = eq + 1;
            size_t n = strlen(v);
            while (n && (v[n - 1] == '\r' || v[n - 1] == '\n')) n--;
            if (n >= sizeof(vals[0])) n = sizeof(vals[0]) - 1;
            memcpy(vals[count], v, n);
            vals[count][n] = 0;
            count++;
        }
        if (f) fclose(f);
    }

    int slot = -1;
    for (int i = 0; i < count; i++) {
        if (strcmp(keys[i], key) == 0) { slot = i; break; }
    }
    if (slot < 0) {
        if (count >= 32) return FALSE;
        slot = count++;
        snprintf(keys[slot], sizeof(keys[slot]), "%s", key);
    }
    snprintf(vals[slot], sizeof(vals[slot]), "%s", value);

    if (_wfopen_s(&f, path, L"w") != 0 || !f) return FALSE;
    for (int i = 0; i < count; i++)
        fprintf(f, "%s=%s\n", keys[i], vals[i]);
    fclose(f);
    return TRUE;
}
