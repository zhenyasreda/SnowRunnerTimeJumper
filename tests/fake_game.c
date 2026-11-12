/*
 * fake_game.c — «имитация игры» для сквозного теста программы.
 *
 * Ведёт себя как SnowRunner в части часов: раз в 16 мс вызывает функцию
 * обновления времени (см. fake_game.S) и печатает текущее время раз в
 * секунду. Собранный файл переименовывается в SnowRunner.exe — тогда
 * программа найдёт его как игру.
 *
 * Запуск:  fake_game.exe
 */
#include <stdio.h>
#include <string.h>
#include <windows.h>

typedef struct {
    char  pad[0x138];
    float clock;
} FakeObject;

extern void fake_update_clock(FakeObject *obj, float value);

int main(void)
{
    FakeObject obj;
    memset(&obj, 0, sizeof(obj));
    obj.clock = 12.0f;

    printf("fake game: pid=%lu старт, время=%.4f\n",
           (unsigned long)GetCurrentProcessId(), (double)obj.clock);
    fflush(stdout);

    for (int i = 0; i < 100000; i++) {
        /* Скорость как в настоящей игре: 1 игровая минута за 2,5 реальной секунды. */
        float t = obj.clock + 0.000111f;
        if (t >= 24.0f) t -= 24.0f;
        fake_update_clock(&obj, t);
        Sleep(16);
        if ((i % 10) == 0) {
            printf("fake game: время=%.4f\n", (double)obj.clock);
            fflush(stdout);
        }
    }
    printf("fake game: выход\n");
    return 0;
}
