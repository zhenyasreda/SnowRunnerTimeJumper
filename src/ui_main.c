/*
 * ui_main.c — окно программы, горячие клавиши, обработка действий.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <windowsx.h>
#include "ui.h"
#include "ui_theme.h"
#include "i18n.h"

static UiState g_state;
static int     g_demo = 0;

/* Базовый размер окна в логических пикселях (96 dpi).
 * Высота включает заголовок окна, поэтому она больше «полезной» высоты
 * клиентской области примерно на 31 пиксель. */
#define BASE_W 560
#define BASE_H 706

/* ------------------------------------------------------------------ */
/*  Режим движения времени                                              */
/* ------------------------------------------------------------------ */

/* Как именно сейчас идёт время: стоит, со своей скоростью, по часам
 * компьютера или в естественном режиме игры. */
typedef enum {
    FLOW_STOPPED = 0,   /* часы остановлены                    */
    FLOW_SPEED,         /* идёт с заданной скоростью           */
    FLOW_SYNC,          /* синхронизировано с часами компьютера */
    FLOW_NATURAL        /* естественный ход времени игры        */
} TimeFlowKind;

typedef struct {
    TimeFlowKind kind;
    double       ms;      /* мс на одну игровую минуту */
} TimeFlow;

static TimeFlow current_flow(UiState *st)
{
    TimeFlow f;
    f.kind = FLOW_STOPPED;
    f.ms = 0.0;
    if (!st->session) return f;
    if (srt_rate_is_sync(st->session)) { f.kind = FLOW_SYNC; return f; }
    if (srt_rate_active(st->session)) {
        f.kind = FLOW_SPEED;
        f.ms = srt_rate_ms_per_minute(st->session);
        return f;
    }
    if ((int)srt_mode(st->session) == SRT_MODE_NATURAL) { f.kind = FLOW_NATURAL; return f; }
    return f;
}

/* Задать новое значение часов, НЕ останавливая течение времени.
 * Если время шло в естественном режиме игры, переходим на обычную
 * скорость: «родной» ход игры всё равно затрёт заданное значение. */
static BOOL apply_value(UiState *st, float hours)
{
    TimeFlow f = current_flow(st);
    switch (f.kind) {
    case FLOW_SPEED:
        return srt_rate_start_at(st->session, f.ms, hours);
    case FLOW_SYNC:
    case FLOW_NATURAL:
        return srt_rate_start_at(st->session, SRT_MS_PER_GAME_MINUTE_DEFAULT, hours);
    default:
        return srt_force_value(st->session, hours);
    }
}

/* ------------------------------------------------------------------ */
/*  Статус и вспомогательные функции                                    */
/* ------------------------------------------------------------------ */

/* Переведённое сообщение в строке состояния. */
static void set_status(UiState *st, StrId id, ...)
{
    const WCHAR *fmt = L(id);
    wchar_t buf[512];
    va_list ap;

    memset(buf, 0, sizeof(buf));
    va_start(ap, id);
    _vsnwprintf(buf, (int)(sizeof(buf) / sizeof(buf[0])) - 1, fmt, ap);
    va_end(ap);
    buf[(int)(sizeof(buf) / sizeof(buf[0])) - 1] = 0;

    wcscpy(st->status, buf);
    st->status_kind = 0;
    /* Перерисовываем только строку состояния — окно не мерцает. */
    if (st->hwnd) InvalidateRect(st->hwnd, &st->rect_status, FALSE);
}

/* Сообщение об ошибке (выделяется цветом). */
static void set_error(UiState *st, StrId id, ...)
{
    const WCHAR *fmt = L(id);
    wchar_t buf[768];
    va_list ap;

    memset(buf, 0, sizeof(buf));
    va_start(ap, id);
    _vsnwprintf(buf, (int)(sizeof(buf) / sizeof(buf[0])) - 1, fmt, ap);
    va_end(ap);
    buf[(int)(sizeof(buf) / sizeof(buf[0])) - 1] = 0;

    wcscpy(st->status, buf);
    st->status_kind = 2;
    if (st->hwnd) InvalidateRect(st->hwnd, &st->rect_status, FALSE);
}

static void refresh_state(UiState *st)
{
    if (!st->session) {
        st->clock_valid = FALSE;
        st->mode = SRT_MODE_NATURAL;
        st->rate_kind = 0;
        return;
    }
    float h = 0.0f;
    st->clock_valid = srt_read_clock(st->session, &h);
    if (st->clock_valid) st->clock_hours = h;
    st->mode = (int)srt_mode(st->session);
    st->rate_kind = srt_rate_is_sync(st->session) ? 2 : (srt_rate_active(st->session) ? 1 : 0);
    st->rate_ms   = srt_rate_ms_per_minute(st->session);
}

/* ------------------------------------------------------------------ */
/*  Геометрия окна                                                      */
/* ------------------------------------------------------------------ */

/* Размер окна в физических пикселях для текущего масштаба. */
static void window_size(const UiState *st, int *w, int *h)
{
    *w = BASE_W * st->dpi / 96;
    *h = BASE_H * st->dpi / 96;
}

static void layout_now(UiState *st)
{
    RECT rc;
    GetClientRect(st->hwnd, &rc);
    ui_layout(st, rc.right, rc.bottom);
}

/* ------------------------------------------------------------------ */
/*  Действия                                                            */
/* ------------------------------------------------------------------ */

static void action_stop(UiState *st)
{
    if (!st->session) { set_status(st, S_ST_NEED_GAME); return; }
    if (srt_set_mode(st->session, SRT_MODE_FROZEN))
        set_status(st, S_ST_STOP_OK);
    else
        set_error(st, S_ST_STOP_FAIL);
    refresh_state(st);
}

static void action_start(UiState *st)
{
    if (!st->session) { set_status(st, S_ST_NEED_GAME); return; }
    if (srt_set_mode(st->session, SRT_MODE_NATURAL))
        set_status(st, S_ST_START_OK);
    else
        set_error(st, S_ST_START_FAIL);
    refresh_state(st);
}

static void action_sync(UiState *st)
{
    if (!st->session) { set_status(st, S_ST_NEED_GAME); return; }
    if (srt_rate_sync(st->session))
        set_status(st, S_ST_SYNC_OK);
    else
        set_error(st, S_ST_SYNC_FAIL);
    refresh_state(st);
}

static void action_speed(UiState *st, float factor)
{
    if (!st->session) { set_status(st, S_ST_NEED_GAME); return; }
    if (factor <= 0.0f) return;
    double ms = 2500.0 / (double)factor;
    if (srt_rate_start(st->session, ms)) {
        if (factor > 1.01f)
            set_status(st, S_ST_SPEED_X, (double)factor);
        else if (factor < 0.99f)
            set_status(st, S_ST_SPEED_DIV, 1.0 / (double)factor);
        else
            set_status(st, S_ST_SPEED_NORMAL);
    } else {
        set_error(st, S_ST_SPEED_FAIL);
    }
    refresh_state(st);
}

static void action_shift(UiState *st, float delta_hours)
{
    if (!st->session) { set_status(st, S_ST_NEED_GAME); return; }

    float cur = 0.0f;
    if (!srt_read_clock(st->session, &cur)) {
        set_error(st, S_ST_READ_FAIL);
        return;
    }
    float next = srt_wrap_hours(cur + delta_hours);
    if (!apply_value(st, next)) {
        set_error(st, S_ST_SET_FAIL);
        return;
    }

    int hh = (int)floor(next);
    int mm = (int)floor((next - hh) * 60.0f + 0.0001f);
    set_status(st, S_ST_SHIFTED, hh, mm);
    refresh_state(st);
}

static void action_set_time(UiState *st, float hours)
{
    if (!st->session) { set_status(st, S_ST_NEED_GAME); return; }
    if (!apply_value(st, hours)) {
        set_error(st, S_ST_SET_FAIL);
        return;
    }
    int hh = (int)floor(hours);
    int mm = (int)floor((hours - hh) * 60.0f + 0.0001f);
    set_status(st, S_ST_TIME_SET, hh, mm);
    refresh_state(st);
}

static void action_attach(UiState *st)
{
    if (st->session) return;
    WCHAR err[768];
    SrtSession *s = NULL;
    SrtStatus rc = srt_attach(&s, err, (int)(sizeof(err) / sizeof(err[0])));
    if (rc != SRT_OK) {
        /* err уже содержит переведённое сообщение. */
        wcscpy(st->status, err[0] ? err : L(S_ERR_GENERIC));
        st->status_kind = 2;
        if (st->hwnd) InvalidateRect(st->hwnd, &st->rect_status, FALSE);
        return;
    }
    st->session = s;
    refresh_state(st);
    set_status(st, S_ST_CONNECTED);
}

static void action_detach(UiState *st)
{
    if (!st->session) return;
    srt_detach(st->session);
    st->session = NULL;
    refresh_state(st);
    set_status(st, S_ST_DISCONNECTED);
    /* Набор кнопок изменился — нужна полная перерисовка. */
    layout_now(st);
    InvalidateRect(st->hwnd, NULL, FALSE);
}

static void action_theme(UiState *st)
{
    UiThemeMode m = ui_theme_mode();
    m = (m == UI_THEME_SYSTEM) ? UI_THEME_LIGHT :
        (m == UI_THEME_LIGHT)  ? UI_THEME_DARK  : UI_THEME_SYSTEM;
    ui_theme_set(m);
    ui_theme_save();
    layout_now(st);
    InvalidateRect(st->hwnd, NULL, FALSE);
    set_status(st, S_ST_THEME_CHANGED);
}

/* Выбор языка интерфейса кнопками «РУС» и «ENG». */
static void set_lang(UiState *st, LangMode mode)
{
    if (lang_mode() == mode && lang_is_explicit()) return;
    lang_set_mode(mode);
    lang_save();
    layout_now(st);
    InvalidateRect(st->hwnd, NULL, FALSE);
    set_status(st, S_ST_LANG_CHANGED);
}

/* ------------------------------------------------------------------ */
/*  Меню точного времени                                                */
/* ------------------------------------------------------------------ */

static void perform(UiState *st, UiAction action, float value)
{
    switch (action) {
    case UI_ACT_STOP:  action_stop(st); break;
    case UI_ACT_START: action_start(st); break;
    case UI_ACT_SYNC:  action_sync(st); break;
    case UI_ACT_SPEED: action_speed(st, value); break;
    case UI_ACT_SHIFT: action_shift(st, value); break;
    case UI_ACT_ATTACH: action_attach(st); break;
    case UI_ACT_DETACH: action_detach(st); break;
    case UI_ACT_THEME:  action_theme(st); break;
    case UI_ACT_LANG_RU: set_lang(st, LANG_RU); break;
    case UI_ACT_LANG_EN: set_lang(st, LANG_EN); break;
    case UI_ACT_PRESET_MENU: break;   /* обрабатывается отдельно */
    }
    layout_now(st);
    InvalidateRect(st->hwnd, NULL, FALSE);
}

static void show_preset_menu(UiState *st, int screen_x, int screen_y)
{
    static const float presets[10] = { 0, 6, 9, 12, 14, 16, 18, 20, 22, 24 };
    HMENU menu = CreatePopupMenu();
    for (int i = 0; i < 10; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%02d:00", (int)presets[i]);
        WCHAR wbuf[32];
        gfx_widen(buf, wbuf, 32);
        AppendMenuW(menu, MF_STRING, 1000 + i, wbuf);
    }
    SetForegroundWindow(st->hwnd);
    UINT cmd = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
                              screen_x, screen_y, 0, st->hwnd, NULL);
    DestroyMenu(menu);
    if (cmd >= 1000 && cmd < 1010) {
        action_set_time(st, presets[cmd - 1000]);
        layout_now(st);
        InvalidateRect(st->hwnd, NULL, FALSE);
    }
}

/* ------------------------------------------------------------------ */
/*  Значок приложения (рисуется в памяти, файлов ресурсов нет)          */
/* ------------------------------------------------------------------ */

static HICON make_app_icon(int size)
{
    HDC screen = GetDC(NULL);
    HDC mem = CreateCompatibleDC(screen);
    HBITMAP color = CreateCompatibleBitmap(screen, size, size);
    HBITMAP old = (HBITMAP)SelectObject(mem, color);

    HBRUSH bg = CreateSolidBrush(RGB(30, 33, 42));
    RECT all = { 0, 0, size, size };
    FillRect(mem, &all, bg);
    DeleteObject(bg);

    HPEN pen = CreatePen(PS_SOLID, size / 14, RGB(88, 142, 255));
    HBRUSH face = CreateSolidBrush(RGB(48, 53, 66));
    HGDIOBJ oldp = SelectObject(mem, pen);
    HGDIOBJ oldb = SelectObject(mem, face);
    Ellipse(mem, size / 8, size / 8, size - size / 8, size - size / 8);
    SelectObject(mem, oldp);
    SelectObject(mem, oldb);
    SelectObject(mem, pen);
    MoveToEx(mem, size / 2, size / 2, NULL);
    LineTo(mem, size / 2, size / 4);
    MoveToEx(mem, size / 2, size / 2, NULL);
    LineTo(mem, size * 3 / 4, size * 2 / 3);
    SelectObject(mem, oldp);
    DeleteObject(pen);
    DeleteObject(face);

    HBITMAP mask = CreateBitmap(size, size, 1, 1, NULL);
    HDC mdc = CreateCompatibleDC(NULL);
    HBITMAP oldm = (HBITMAP)SelectObject(mdc, mask);
    PatBlt(mdc, 0, 0, size, size, WHITENESS);
    HBRUSH black = CreateSolidBrush(RGB(0, 0, 0));
    HGDIOBJ om = SelectObject(mdc, black);
    HGDIOBJ op = SelectObject(mdc, GetStockObject(NULL_PEN));
    Ellipse(mdc, size / 8, size / 8, size - size / 8, size - size / 8);
    SelectObject(mdc, om);
    SelectObject(mdc, op);
    SelectObject(mdc, oldm);
    DeleteDC(mdc);
    DeleteObject(black);

    ICONINFO ii;
    memset(&ii, 0, sizeof(ii));
    ii.fIcon = TRUE;
    ii.hbmColor = color;
    ii.hbmMask = mask;
    HICON icon = CreateIconIndirect(&ii);

    SelectObject(mem, old);
    DeleteDC(mem);
    DeleteObject(color);
    DeleteObject(mask);
    ReleaseDC(NULL, screen);
    return icon;
}

/* ------------------------------------------------------------------ */
/*  Шрифты                                                              */
/* ------------------------------------------------------------------ */

BOOL ui_fonts_create(UiState *st)
{
    int d = st->dpi;
    st->f_title  = gfx_font_create(d * 19 / 96, 1);
    st->f_sub    = gfx_font_create(d * 12 / 96, 0);
    st->f_clock  = gfx_font_create(d * 46 / 96, 0);
    st->f_chip   = gfx_font_create(d * 12 / 96, 1);
    st->f_btn    = gfx_font_create(d * 13 / 96, 0);
    st->f_sect   = gfx_font_create(d * 11 / 96, 1);
    st->f_key    = gfx_font_create(d * 12 / 96, 1);
    st->f_desc   = gfx_font_create(d * 12 / 96, 0);
    st->f_status = gfx_font_create(d * 12 / 96, 0);
    return st->f_title && st->f_clock && st->f_btn;
}

void ui_fonts_destroy(UiState *st)
{
    gfx_font_destroy(st->f_title);  st->f_title = NULL;
    gfx_font_destroy(st->f_sub);    st->f_sub = NULL;
    gfx_font_destroy(st->f_clock);  st->f_clock = NULL;
    gfx_font_destroy(st->f_chip);   st->f_chip = NULL;
    gfx_font_destroy(st->f_btn);    st->f_btn = NULL;
    gfx_font_destroy(st->f_sect);   st->f_sect = NULL;
    gfx_font_destroy(st->f_key);    st->f_key = NULL;
    gfx_font_destroy(st->f_desc);   st->f_desc = NULL;
    gfx_font_destroy(st->f_status); st->f_status = NULL;
}

/* ------------------------------------------------------------------ */
/*  DPI                                                                 */
/* ------------------------------------------------------------------ */

/* DPI окна (работает на всех версиях Windows). */
static int window_dpi(HWND hwnd)
{
    HDC hdc = GetDC(hwnd);
    int dpi = 96;
    if (hdc) {
        int d = GetDeviceCaps(hdc, LOGPIXELSY);
        if (d > 0) dpi = d;
        ReleaseDC(hwnd, hdc);
    }
    return dpi;
}

/* DPI-осведомлённость: включаем программно, чтобы не зависеть от манифеста. */
static void enable_dpi_awareness(void)
{
    typedef BOOL (WINAPI *SetCtxFn)(HANDLE);
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    BOOL ok = FALSE;
    if (user32) {
        SetCtxFn fn = (SetCtxFn)(uintptr_t)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
        if (fn) {
            /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 */
            if (fn((HANDLE)(LONG_PTR)-4)) ok = TRUE;
            /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE */
            else if (fn((HANDLE)(LONG_PTR)-3)) ok = TRUE;
        }
    }
    if (!ok) SetProcessDPIAware();
}

/* ------------------------------------------------------------------ */
/*  Окно                                                                */
/* ------------------------------------------------------------------ */

static const WCHAR CLASS_NAME[] = L"SnowRunnerTimeJumperWindow";

static void register_hotkeys(HWND hwnd)
{
    for (int i = 0; i < g_hotkey_count; i++) {
        const HotkeyDef *hk = &g_hotkeys[i];
        if (!RegisterHotKey(hwnd, hk->id, hk->mods | MOD_NOREPEAT, hk->vk)) {
            WCHAR wkey[64];
            gfx_widen(hk->keys, wkey, 64);
            set_status(&g_state, S_ST_HOTKEY_TAKEN, wkey);
        }
    }
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    UiState *st = &g_state;

    switch (msg) {
    case WM_CREATE: {
        st->hwnd = hwnd;
        st->demo = g_demo;
        if (st->demo) {
            st->clock_valid = TRUE;
            st->clock_hours = 14.5f;
            st->mode = SRT_MODE_NATURAL;
            st->rate_kind = 1;
            st->rate_ms = 2500.0;
        }
        st->dpi  = window_dpi(hwnd);
        ui_fonts_create(st);
        {
            int w, h;
            window_size(st, &w, &h);
            SetWindowPos(hwnd, NULL, 0, 0, w, h,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        layout_now(st);
        register_hotkeys(hwnd);
        SetTimer(hwnd, 1, 50, NULL);
        action_attach(st);          /* пытаемся подключиться сразу */
        layout_now(st);
        return 0;
    }

    case WM_TIMER: {
        if (wParam == 1) {
            if (st->session) {
                srt_rate_tick(st->session);
                refresh_state(st);
                if (!st->clock_valid) {
                    /* Игра закрылась или память стала недоступна. */
                    if (++st->read_fail > 10) {
                        action_detach(st);
                        set_status(st, S_ST_GAME_CLOSED);
                        st->read_fail = 0;
                    }
                } else {
                    st->read_fail = 0;
                }
            }

            /* Перерисовываем только изменившиеся области: иначе окно
             * мерцает и постоянно нагружает процессор. */
            WCHAR text[32];
            memset(text, 0, sizeof(text));
            if (st->clock_valid)
                ui_format_clock(st->clock_hours, text, 32);
            else
                wcscpy(text, L"--:--");
            int mode_key = st->mode * 10 + st->rate_kind;
            if (wcscmp(text, st->shown_clock) != 0) {
                wcscpy(st->shown_clock, text);
                InvalidateRect(hwnd, &st->rect_clock, FALSE);
            }
            if (mode_key != st->shown_mode) {
                st->shown_mode = mode_key;
                InvalidateRect(hwnd, &st->rect_chip, FALSE);
                InvalidateRect(hwnd, &st->rect_clock, FALSE);
            }
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        ui_paint(st, hdc, rc.right, rc.bottom);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;                    /* фон рисуем сами */

    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        int hover = 0;
        for (int i = 0; i < st->button_count; i++) {
            POINT p = { x, y };
            if (PtInRect(&st->buttons[i].rect, p)) { hover = st->buttons[i].id; break; }
        }
        if (hover != st->hover_id) {
            st->hover_id = hover;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        tme.dwHoverTime = 0;
        TrackMouseEvent(&tme);
        return 0;
    }

    case WM_MOUSELEAVE: {
        if (st->hover_id) { st->hover_id = 0; InvalidateRect(hwnd, NULL, FALSE); }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        POINT p = { x, y };
        for (int i = 0; i < st->button_count; i++) {
            UiButton *b = &st->buttons[i];
            if (!PtInRect(&b->rect, p)) continue;
            if (b->action == UI_ACT_PRESET_MENU) {
                POINT tl = { b->rect.left, b->rect.bottom };
                ClientToScreen(hwnd, &tl);
                show_preset_menu(st, tl.x, tl.y);
            } else {
                perform(st, b->action, b->value);
            }
            break;
        }
        return 0;
    }

    case WM_HOTKEY: {
        const HotkeyDef *hk = hotkey_by_id((int)wParam);
        if (!hk) return 0;
        switch (hk->action) {
        case ACT_STOP:     perform(st, UI_ACT_STOP, 0); break;
        case ACT_START:    perform(st, UI_ACT_START, 0); break;
        case ACT_SYNC:     perform(st, UI_ACT_SYNC, 0); break;
        case ACT_SPEED:    perform(st, UI_ACT_SPEED, hk->value); break;
        case ACT_SHIFT:    perform(st, UI_ACT_SHIFT, hk->value); break;
        case ACT_SET_TIME: action_set_time(st, hk->value); layout_now(st);
                          InvalidateRect(hwnd, NULL, FALSE); break;
        }
        return 0;
    }

    case WM_SETTINGCHANGE: {
        /* Системная тема или язык системы могли измениться. */
        if (ui_theme_mode() == UI_THEME_SYSTEM) {
            ui_theme_set(UI_THEME_SYSTEM);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        /* Язык системы учитывается, только пока пользователь не выбрал
         * язык вручную кнопкой «РУС»/«ENG». */
        if (!lang_is_explicit()) {
            lang_follow_system();
            layout_now(st);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_DPICHANGED: {
        int newdpi = HIWORD(wParam);
        if (newdpi > 0) st->dpi = newdpi;
        ui_fonts_destroy(st);
        ui_fonts_create(st);
        int w, h;
        window_size(st, &w, &h);
        RECT *r = (RECT *)lParam;
        int x = r ? r->left : 0;
        int y = r ? r->top : 0;
        SetWindowPos(hwnd, NULL, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
        layout_now(st);
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }

    case WM_SIZE: {
        layout_now(st);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        for (int i = 0; i < g_hotkey_count; i++) UnregisterHotKey(hwnd, g_hotkeys[i].id);
        if (st->session) {
            srt_detach(st->session);
            st->session = NULL;
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* ------------------------------------------------------------------ */
/*  Диагностика: отрисовать окно в файл BMP (не снимая экран).         */
/* ------------------------------------------------------------------ */

static BOOL save_window_image(HWND hwnd, const char *path)
{
    RECT rc;
    if (!GetClientRect(hwnd, &rc)) return FALSE;
    int w = rc.right, h = rc.bottom;
    if (w <= 0 || h <= 0) return FALSE;

    HDC screen = GetDC(NULL);
    HDC mem = CreateCompatibleDC(screen);
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void *bits = NULL;
    HBITMAP dib = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!dib || !bits) {
        if (dib) DeleteObject(dib);
        DeleteDC(mem);
        ReleaseDC(NULL, screen);
        return FALSE;
    }
    HBITMAP old = (HBITMAP)SelectObject(mem, dib);
    ui_paint(&g_state, mem, w, h);
    GdiFlush();

    BITMAPFILEHEADER filehdr;
    memset(&filehdr, 0, sizeof(filehdr));
    filehdr.bfType = 0x4D42;
    filehdr.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    filehdr.bfSize = filehdr.bfOffBits + (DWORD)w * (DWORD)h * 4;

    BOOL ok = FALSE;
    HANDLE f = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    if (f != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        ok = WriteFile(f, &filehdr, sizeof(filehdr), &written, NULL) &&
             WriteFile(f, &bmi, sizeof(bmi), &written, NULL) &&
             WriteFile(f, bits, (DWORD)w * (DWORD)h * 4, &written, NULL);
        CloseHandle(f);
    }
    SelectObject(mem, old);
    DeleteObject(dib);
    DeleteDC(mem);
    ReleaseDC(NULL, screen);
    return ok;
}

/* ------------------------------------------------------------------ */
/*  Точка входа                                                         */
/* ------------------------------------------------------------------ */

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hprev, LPSTR cmdline, int show)
{
    (void)hprev; (void)cmdline;

    enable_dpi_awareness();

    memset(&g_state, 0, sizeof(g_state));
    g_state.dpi = 96;
    g_demo = (strstr(cmdline, "--demo") != NULL);

    if (!gfx_init()) {
        MessageBoxW(NULL, L(S_ERR_GFX), L(S_APP_TITLE), MB_ICONERROR);
        return 1;
    }

    /* тема оформления и язык интерфейса: по умолчанию — как в системе */
    ui_theme_set(UI_THEME_SYSTEM);
    ui_theme_load();
    lang_load();

    WNDCLASSEXW wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc   = wnd_proc;
    wc.hInstance     = hinst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszClassName = CLASS_NAME;
    if (!RegisterClassExW(&wc)) {
        gfx_shutdown();
        return 1;
    }

    int w = BASE_W, h = BASE_H;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L(S_APP_TITLE),
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                sx, sy, w, h, NULL, NULL, hinst, NULL);
    if (!hwnd) {
        gfx_shutdown();
        return 1;
    }

    HICON icon = make_app_icon(64);
    if (icon) {
        SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)icon);
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
    }

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    /* Диагностический режим: снимок окна в BMP-файл и выход.
     * Запуск: SnowRunnerTimeJumper.exe --snapshot=путь\к\файлу.bmp */
    {
        const char *opt = strstr(cmdline, "--snapshot=");
        if (opt) {
            char file[MAX_PATH];
            size_t n = 0;
            opt += strlen("--snapshot=");
            if (*opt == '"') opt++;                 /* путь может быть в кавычках */
            while (opt[n] && n + 1 < sizeof(file) && opt[n] != '"') { file[n] = opt[n]; n++; }
            file[n] = 0;
            Sleep(1500);
            {
                WCHAR wfile[MAX_PATH];
                gfx_widen(file, wfile, MAX_PATH);
                if (save_window_image(hwnd, file))
                    log_msg(S_LOG_SNAPSHOT_OK, wfile);
                else
                    log_msg(S_LOG_SNAPSHOT_FAIL, wfile);
            }
            DestroyWindow(hwnd);
            if (icon) DestroyIcon(icon);
            ui_fonts_destroy(&g_state);
            gfx_shutdown();
            return 0;
        }
    }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (icon) DestroyIcon(icon);
    ui_fonts_destroy(&g_state);
    gfx_shutdown();
    return (int)msg.wParam;
}
