/*
 * srt_log.c — журнал работы программы.
 *
 * Пишет UTF-8 текст в %LOCALAPPDATA%\SnowRunnerTimeJumper\srtc.log.
 * Никаких других файлов программа не создаёт и не изменяет.
 */
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include "srt.h"

static void log_path(wchar_t *buf, size_t size)
{
    wchar_t local[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", local, MAX_PATH);
    if (n == 0 || n > MAX_PATH) {
        wcscpy(local, L".");
    }
    _snwprintf(buf, size, L"%s\\SnowRunnerTimeJumper", local);
    CreateDirectoryW(buf, NULL);
    _snwprintf(buf, size, L"%s\\SnowRunnerTimeJumper\\srtc.log", local);
}

void srt_log(const char *fmt, ...)
{
    wchar_t path[MAX_PATH];
    log_path(path, sizeof(path) / sizeof(path[0]));

    FILE *f = NULL;
    /* Режим "a" без указания кодировки: текст пишется как есть (UTF-8),
     * а добавление "ccs=UTF-8" в некоторых сборках CRT приводит к потере
     * записей. */
    if (_wfopen_s(&f, path, L"a") != 0 || !f) {
        f = NULL;
    }
    if (!f) {
        fopen_s(&f, "srtc.log", "a");
        if (!f) return;
    }

    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(f, "%02d:%02d:%02d.%03d  ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);

    fputc('\n', f);
    fclose(f);
}
