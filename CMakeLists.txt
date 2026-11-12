cmake_minimum_required(VERSION 3.16)
project(SnowRunnerTimeJumper LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8 /W3 /wd4100)
    # Пользовательский манифест встраивается сам, автоматический — отключаем.
    add_link_options(/MANIFEST:NO)
else()
    add_compile_options(-Wall -Wextra -std=gnu11)
    add_link_options(-mwindows)
endif()

file(GLOB SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

add_executable(SnowRunnerTimeJumper WIN32 ${SRC_FILES})

if(MSVC)
    # Манифест нужен только под MSVC: в MinGW он конфликтует со встроенным,
    # а DPI-осведомлённость там включается программно.
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/app.manifest" MANIFEST_PATH)
    string(REPLACE "\\" "/" MANIFEST_PATH "${MANIFEST_PATH}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/app_generated.rc"
         "1 RT_MANIFEST \"${MANIFEST_PATH}\"\n")
    target_sources(SnowRunnerTimeJumper PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/app_generated.rc")
endif()

target_link_libraries(SnowRunnerTimeJumper PRIVATE gdi32 gdiplus user32 shell32)

set_target_properties(SnowRunnerTimeJumper PROPERTIES
    OUTPUT_NAME "SnowRunnerTimeJumper"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

# Автотест машинного кода стаба (нужен GCC: используется ассемблерный файл).
if(NOT MSVC)
    enable_testing()
    enable_language(ASM)
    add_executable(stub_test tests/stub_test.c tests/call_stub.S src/srt_stub.c)
    target_include_directories(stub_test PRIVATE src)
    add_test(NAME stub_test COMMAND stub_test)
endif()
