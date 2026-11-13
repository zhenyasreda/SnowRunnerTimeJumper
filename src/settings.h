/*
 * settings.h — файл настроек программы (%LOCALAPPDATA%\SnowRunnerTimeJumper\settings.ini).
 *
 * Простой формат «ключ=значение» в кодировке UTF-8.
 */
#ifndef SETTINGS_H_INCLUDED
#define SETTINGS_H_INCLUDED

#include <windows.h>

/* Прочитать значение ключа. Возвращает FALSE, если ключа нет.
 * В out копируется строка UTF-8 без завершающего нуля. */
BOOL settings_get(const char *key, char *out, int out_size);

/* Записать значение ключа (файл переписывается целиком). */
BOOL settings_set(const char *key, const char *value);

#endif /* SETTINGS_H_INCLUDED */
