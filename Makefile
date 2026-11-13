# Сборка SnowRunner Time Jumper (GCC / MinGW-w64).
#
#   mingw32-make            — обычная сборка
#   mingw32-make test       — сборка и запуск автотеста машинного кода
#   mingw32-make clean      — очистка
#
# Выходные файлы: build/SnowRunnerTimeJumper.exe

ifeq ($(origin CC),default)
CC := gcc
endif
CFLAGS   ?= -O2 -Wall -Wextra -std=gnu11
LDFLAGS  ?= -mwindows
LDLIBS   ?= -lgdi32 -lgdiplus -luser32 -lshell32

ifeq ($(OS),Windows_NT)
# Рецепты рассчитаны на cmd.exe. Если в PATH есть sh.exe (например, из
# Git for Windows или на CI-раннере GitHub), GNU make выбрал бы его — и
# команды вроде «mkdir build\obj» отработали бы неверно. Поэтому оболочку
# задаём явно.
SHELL   = cmd.exe
MKDIR   = mkdir $(BUILD) 2>nul & mkdir $(BUILD)\obj 2>nul & exit 0
CLEAN   = if exist $(BUILD) rmdir /s /q $(BUILD) & exit 0
else
MKDIR = mkdir -p $(BUILD) $(OBJ)
CLEAN = rm -rf $(BUILD)
endif

BUILD    := build
OBJ      := $(BUILD)/obj
TARGET   := $(BUILD)/SnowRunnerTimeJumper.exe
TEST     := $(BUILD)/stub_test.exe

SRCS     := $(wildcard src/*.c)
OBJS     := $(patsubst src/%.c,$(OBJ)/%.o,$(SRCS))

# Зависимости от заголовков отслеживаются автоматически (-MMD -MP):
# изменение i18n.h пересобирает все файлы, которые его используют.
DEPFLAGS := -MMD -MP

.PHONY: all test e2e clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@$(MKDIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) -static-libgcc
	@echo.
	@echo Готово: $(TARGET)

$(OBJ)/%.o: src/%.c
	@$(MKDIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(OBJS:.o=.d)

# Манифест с DPI-осведомлённостью в сборке MinGW не нужен:
# он включается программно (SetProcessDpiAwarenessContext),
# а встроенный в MinGW манифест конфликтует с пользовательским.

test: $(TEST)
	$(TEST)

$(TEST): tests/stub_test.c tests/call_stub.S src/srt_stub.c src/srt_priv.h src/srt.h
	@$(MKDIR)
	$(CC) $(CFLAGS) -Isrc -o $@ tests/stub_test.c tests/call_stub.S src/srt_stub.c -mconsole

# Имитация игры для сквозного теста: в ней ровно та же инструкция
# обновления часов, что и в настоящей игре.
$(BUILD)/fake_game.exe: tests/fake_game.c tests/fake_game.S
	@$(MKDIR)
	$(CC) $(CFLAGS) -o $@ tests/fake_game.c tests/fake_game.S -mconsole

e2e: $(BUILD)/fake_game.exe $(TARGET)
	powershell -NoProfile -ExecutionPolicy Bypass -File tests\\e2e_test.ps1

clean:
	@$(CLEAN)
