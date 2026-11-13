/*
 * i18n.h — локализация интерфейса.
 *
 * Программа стартует на языке системы: если он русский — на русском,
 * иначе на английском. В окне есть две кнопки — «РУС» и «ENG».
 */

#ifndef I18N_H_INCLUDED
#define I18N_H_INCLUDED

#include <windows.h>

/* Язык интерфейса. Поддерживаются ровно два. */
typedef enum {
    LANG_RU = 0,        /* русский    */
    LANG_EN             /* английский */
} LangMode;

/* Идентификаторы строк. Порядок должен совпадать с таблицей в i18n.c. */
typedef enum {
    /* --- общее --- */
    S_APP_TITLE,
    S_HEADER,
    S_CONNECTED_NO,
    S_MEMORY_CHANGED,
    S_SECTION_TIME,
    S_SECTION_KEYS,

    /* --- карточка с часами --- */
    S_CHIP_NO_GAME,
    S_CHIP_NORMAL,
    S_CHIP_STOP,
    S_CHIP_SYNC,
    S_DESC_NO_GAME,
    S_DESC_NORMAL,
    S_DESC_STOP,
    S_DESC_SYNC,
    S_DESC_SPEED,

    /* --- кнопки --- */
    S_BTN_STOP,
    S_BTN_START,
    S_BTN_SYNC,
    S_BTN_CONNECT,
    S_BTN_DISCONNECT,
    S_BTN_EXACT,
    S_SHIFT_M1,
    S_SHIFT_P1,
    S_SHIFT_M2,
    S_SHIFT_P2,

    /* --- тема и язык --- */
    S_THEME_SYSTEM,
    S_THEME_LIGHT,
    S_THEME_DARK,
    S_LANG_RU,          /* подпись кнопки «РУС»  */
    S_LANG_EN,          /* подпись кнопки «ENG»  */

    /* --- сообщения в строке состояния --- */
    S_ST_CONNECTED,
    S_ST_DISCONNECTED,
    S_ST_GAME_CLOSED,
    S_ST_STOP_OK,
    S_ST_START_OK,
    S_ST_SYNC_OK,
    S_ST_SPEED_X,
    S_ST_SPEED_DIV,
    S_ST_SPEED_NORMAL,
    S_ST_SHIFTED,
    S_ST_TIME_SET,
    S_ST_NEED_GAME,
    S_ST_READ_FAIL,
    S_ST_SET_FAIL,
    S_ST_STOP_FAIL,
    S_ST_START_FAIL,
    S_ST_SYNC_FAIL,
    S_ST_SPEED_FAIL,
    S_ST_THEME_CHANGED,
    S_ST_LANG_CHANGED,
    S_ST_HOTKEY_TAKEN,

    /* --- ошибки подключения --- */
    S_ERR_NOT_FOUND,
    S_ERR_OPEN,
    S_ERR_MODULE,
    S_ERR_PATTERN,
    S_ERR_AMBIGUOUS,
    S_ERR_ALLOC,
    S_ERR_IO,
    S_ERR_PATCH,
    S_ERR_GFX,
    S_ERR_GENERIC,

    /* --- журнал --- */
    S_LOG_DPI,
    S_LOG_SCAN_EXACT,
    S_LOG_SCAN_ADDR,
    S_LOG_SCAN_FREE,
    S_LOG_ATTACH_PID,
    S_LOG_ATTACH_CAND,
    S_LOG_ATTACH_BLOCK,
    S_LOG_ATTACH_OK,
    S_LOG_ATTACH_RETRY,
    S_LOG_PATCH_BYTES,
    S_LOG_PATCH_MISMATCH,
    S_LOG_VERIFY,
    S_LOG_VERIFY_BAD,
    S_LOG_DETACH,
    S_LOG_GFX_FAMILY,
    S_LOG_GFX_FONT,
    S_LOG_SNAPSHOT_OK,
    S_LOG_SNAPSHOT_FAIL,

    /* --- описания горячих клавиш (в порядке таблицы hotkeys.c) --- */
    S_HK_STOP,
    S_HK_START,
    S_HK_SPEED_1,
    S_HK_SPEED_2,
    S_HK_SPEED_3,
    S_HK_SPEED_4,
    S_HK_SLOW_2,
    S_HK_SLOW_3,
    S_HK_SLOW_4,
    S_HK_BACK_1,
    S_HK_FWD_1,
    S_HK_FWD_2,
    S_HK_BACK_2,
    S_HK_T_00,
    S_HK_T_06,
    S_HK_T_09,
    S_HK_T_12,
    S_HK_T_14,
    S_HK_T_16,
    S_HK_T_18,
    S_HK_T_20,
    S_HK_T_22,
    S_HK_T_24,
    S_HK_SYNC,

    S_COUNT
} StrId;

/* Текущий язык интерфейса. */
void     lang_set_mode(LangMode mode);
LangMode lang_mode(void);

/* Индекс выбранного языка: 0 — русский, 1 — английский. */
int      lang_index(void);

/* Перевод строки по идентификатору. */
const WCHAR *L(StrId id);

/* Отформатировать переведённую строку в буфер (всегда с завершающим нулём). */
void lang_format(WCHAR *buf, int chars, StrId id, ...);

/* Русский ли язык системы по умолчанию? */
BOOL   lang_system_is_russian(void);

/* Выбран ли язык вручную? Если нет — программа следует за языком системы. */
BOOL   lang_is_explicit(void);

/* Загрузить/сохранить язык из файла настроек. */
void   lang_load(void);
void   lang_save(void);

/* Вернуться к языку системы (сброс ручного выбора). */
void   lang_follow_system(void);

/* Запись переведённой строки в журнал. */
void   log_msg(StrId id, ...);

#endif /* I18N_H_INCLUDED */
