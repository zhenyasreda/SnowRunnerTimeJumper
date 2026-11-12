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
# В Windows make выполняет команды через cmd.exe, поэтому пути с обратными
# косыми чертами и без Unix-опций у mkdir.
MKDIR = mkdir $(BUILD) 2>nul & mkdir $(BUILD)\obj 2>nul & exit 0
CLEAN = if exist $(BUILD) rmdir /s /q $(BUILD) & exit 0
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
HDRS     := src/srt.h src/srt_priv.h src/ui.h src/ui_gfx.h src/hotkeys.h

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@$(MKDIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) -static-libgcc
	@echo.
	@echo Готово: $(TARGET)

$(OBJ)/%.o: src/%.c $(HDRS)
	@$(MKDIR)
	$(CC) $(CFLAGS) -c $< -o $@

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
	copy /y $(BUILD)\fake_game.exe $(BUILD)\SnowRunner.exe >nul
	@cmd /c "tests\\e2e_test.bat"

clean:
	@$(CLEAN)
