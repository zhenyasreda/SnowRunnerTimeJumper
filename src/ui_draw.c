/*
 * ui_draw.c — геометрия и отрисовка окна.
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ui.h"
#include "ui_theme.h"

/* ----------------------------- геометрия ---------------------------- */

typedef struct {
    int pad, x, w;
    int title_y, sub_y;
    int card_x, card_y, card_w, card_h;
    int clock_x, clock_y, clock_w, clock_h;
    int mode_x, mode_y, mode_w, mode_h;
    int chip_x, chip_y, chip_w, chip_h;
    int theme_x, theme_y, theme_w, theme_h;
    int sect_y, row_y[3], row_h, row_gap;
    int list_y, list_row_h, key_w, col_w;
    int status_y, status_w;
} UiMetrics;

static int px(const UiState *st, int v)
{
    return v * st->dpi / 96;
}

static void metrics(const UiState *st, int width, int height, UiMetrics *m)
{
    (void)height;
    m->pad      = px(st, 22);
    m->x        = m->pad;
    m->w        = width - 2 * m->pad;
    m->title_y  = px(st, 14);
    m->sub_y    = px(st, 42);
    m->card_x   = m->pad;
    m->card_y   = px(st, 70);
    m->card_w   = m->w;
    m->card_h   = px(st, 108);
    m->clock_x  = m->pad + px(st, 22);
    m->clock_y  = m->card_y + px(st, 10);
    m->clock_w  = px(st, 290);
    m->clock_h  = px(st, 58);
    m->mode_x   = m->clock_x;
    m->mode_y   = m->card_y + px(st, 72);
    m->mode_w   = px(st, 300);
    m->mode_h   = px(st, 20);
    m->chip_x   = width - m->pad - px(st, 106);
    m->chip_y   = m->card_y + px(st, 38);
    m->chip_w   = px(st, 94);
    m->chip_h   = px(st, 30);
    m->theme_w  = px(st, 104);
    m->theme_h  = px(st, 26);
    m->theme_x  = width - m->pad - m->theme_w;
    m->theme_y  = px(st, 12);
    m->sect_y   = px(st, 194);
    m->row_h    = px(st, 38);
    m->row_gap  = px(st, 8);
    m->row_y[0] = px(st, 218);
    m->row_y[1] = m->row_y[0] + m->row_h + m->row_gap;
    m->row_y[2] = m->row_y[1] + m->row_h + m->row_gap;
    m->list_y   = px(st, 388);
    m->list_row_h = px(st, 20);
    m->key_w    = px(st, 104);
    m->col_w    = (m->w - px(st, 12)) / 2;
    m->status_y = px(st, 636);
    m->status_w = m->w - px(st, 108);
}

/* ----------------------------- кнопки ------------------------------ */

static void add_button(UiState *st, int x, int y, int w, int h,
                       UiAction action, float value,
                       const WCHAR *label, UiButtonStyle style)
{
    if (st->button_count >= (int)(sizeof(st->buttons) / sizeof(st->buttons[0]))) return;
    UiButton *b = &st->buttons[st->button_count];
    b->rect.left = x;
    b->rect.top  = y;
    b->rect.right = x + w;
    b->rect.bottom = y + h;
    b->action = action;
    b->value  = value;
    b->style  = style;
    b->id     = st->button_count + 1;
    wcsncpy(b->label, label, 31);
    b->label[31] = 0;
    st->button_count++;
}

static void add_row3(UiState *st, const UiMetrics *m, const WCHAR *l1, UiAction a1, float v1,
                     const WCHAR *l2, UiAction a2, float v2,
                     const WCHAR *l3, UiAction a3, float v3,
                     const WCHAR *l4, UiAction a4, float v4,
                     const WCHAR *l5, UiAction a5)
{
    int gap = px(st, 8);
    int small = px(st, 78);
    int x = m->x;
    add_button(st, x, m->row_y[2], small, m->row_h, a1, v1, l1, UI_BTN_NORMAL); x += small + gap;
    add_button(st, x, m->row_y[2], small, m->row_h, a2, v2, l2, UI_BTN_NORMAL); x += small + gap;
    add_button(st, x, m->row_y[2], small, m->row_h, a3, v3, l3, UI_BTN_NORMAL); x += small + gap;
    add_button(st, x, m->row_y[2], small, m->row_h, a4, v4, l4, UI_BTN_NORMAL); x += small + gap;
    int rest = m->x + m->w - x;
    add_button(st, x, m->row_y[2], rest, m->row_h, a5, 0.0f, l5, UI_BTN_NORMAL);
}

void ui_layout(UiState *st, int width, int height)
{
    UiMetrics m;
    metrics(st, width, height, &m);
    st->button_count = 0;

    /* Области, которые перерисовываются при изменении состояния. */
    st->rect_clock.left   = m.mode_x;
    st->rect_clock.top    = m.clock_y - px(st, 4);
    st->rect_clock.right  = m.mode_x + m.mode_w;
    st->rect_clock.bottom = m.mode_y + m.mode_h;
    st->rect_chip.left    = m.chip_x;
    st->rect_chip.top     = m.chip_y;
    st->rect_chip.right   = m.chip_x + m.chip_w;
    st->rect_chip.bottom  = m.chip_y + m.chip_h;
    st->rect_status.left  = m.x;
    st->rect_status.top   = m.status_y;
    st->rect_status.right = m.x + m.w;
    st->rect_status.bottom = m.status_y + px(st, 34);

    /* переключатель темы в шапке */
    add_button(st, m.theme_x, m.theme_y, m.theme_w, m.theme_h,
               UI_ACT_THEME, 0.0f, ui_theme_name(), UI_BTN_NORMAL);

    if (!st->session && !st->demo) {
        add_button(st, m.x, m.row_y[0], m.w, m.row_h,
                   UI_ACT_ATTACH, 0.0f, L"Подключиться к игре", UI_BTN_ACCENT);
        return;
    }

    /* ряд 1: стоп / пуск / по часам */
    int gap = px(st, 10);
    int bw = (m.w - 2 * gap) / 3;
    int x = m.x;
    add_button(st, x, m.row_y[0], bw, m.row_h, UI_ACT_STOP,  0.0f, L"Остановить", UI_BTN_NORMAL); x += bw + gap;
    add_button(st, x, m.row_y[0], bw, m.row_h, UI_ACT_START, 0.0f, L"Запустить",  UI_BTN_NORMAL); x += bw + gap;
    add_button(st, x, m.row_y[0], bw, m.row_h, UI_ACT_SYNC,  0.0f, L"По часам",   UI_BTN_NORMAL);

    /* ряд 2: скорости (обычная, x2, x3, x4, /2, /3, /4) */
    gap = px(st, 8);
    bw = (m.w - 6 * gap) / 7;
    x = m.x;
    add_button(st, x, m.row_y[1], bw, m.row_h, UI_ACT_SPEED, 1.0f,      L"×1", UI_BTN_NORMAL); x += bw + gap;
    add_button(st, x, m.row_y[1], bw, m.row_h, UI_ACT_SPEED, 2.0f,      L"×2", UI_BTN_NORMAL); x += bw + gap;
    add_button(st, x, m.row_y[1], bw, m.row_h, UI_ACT_SPEED, 3.0f,      L"×3", UI_BTN_NORMAL); x += bw + gap;
    add_button(st, x, m.row_y[1], bw, m.row_h, UI_ACT_SPEED, 4.0f,      L"×4", UI_BTN_NORMAL); x += bw + gap;
    add_button(st, x, m.row_y[1], bw, m.row_h, UI_ACT_SPEED, 0.5f,      L"÷2", UI_BTN_NORMAL); x += bw + gap;
    add_button(st, x, m.row_y[1], bw, m.row_h, UI_ACT_SPEED, 0.33334f,  L"÷3", UI_BTN_NORMAL); x += bw + gap;
    add_button(st, x, m.row_y[1], bw, m.row_h, UI_ACT_SPEED, 0.25f,     L"÷4", UI_BTN_NORMAL);

    /* ряд 3: сдвиг часов и точное время */
    add_row3(st, &m,
             L"−1 ч", UI_ACT_SHIFT, -1.0f,
             L"+1 ч", UI_ACT_SHIFT,  1.0f,
             L"−2 ч", UI_ACT_SHIFT, -2.0f,
             L"+2 ч", UI_ACT_SHIFT,  2.0f,
             L"Точное время ▾", UI_ACT_PRESET_MENU);

    /* отключение */
    add_button(st, m.x + m.w - px(st, 100), m.status_y - px(st, 3), px(st, 100), px(st, 24),
               UI_ACT_DETACH, 0.0f, L"Отключиться", UI_BTN_NORMAL);
}

/* --------------------------- отрисовка --------------------------- */

/* Форматирование времени без printf: результат всегда завершён нулём
 * (в некоторых сборках CRT swprintf этого не гарантирует). */
void ui_format_clock(float hours, WCHAR *out, int n)
{
    if (!out || n < 6) return;
    memset(out, 0, (size_t)n * sizeof(WCHAR));

    double h = (double)hours;
    int hh = (int)floor(h);
    int mm = (int)floor((h - hh) * 60.0 + 0.0001);
    if (mm >= 60) { mm -= 60; hh += 1; }
    if (hh < 0) hh = 0;
    if (hh > 24) hh = 24;

    out[0] = (WCHAR)('0' + (hh / 10) % 10);
    out[1] = (WCHAR)('0' + hh % 10);
    out[2] = (WCHAR)':';
    out[3] = (WCHAR)('0' + (mm / 10) % 10);
    out[4] = (WCHAR)('0' + mm % 10);
    out[5] = 0;
}

static void mode_info(const UiState *st, WCHAR *chip, int chip_n,
                      WCHAR *desc, int desc_n, COLORREF *chip_color)
{
    const UiPalette *p = ui_theme_palette();
    (void)desc_n;

    if (!st->session) {
        wcscpy(chip, L"Нет игры");
        wcscpy(desc, L"Запустите SnowRunner и нажмите «Подключиться»");
        *chip_color = p->dim;
        return;
    }
    if (st->rate_kind == 2) {
        wcscpy(chip, L"По часам");
        wcscpy(desc, L"Время синхронизировано с часами компьютера");
        *chip_color = p->warn;
        return;
    }
    if (st->rate_kind == 1 && st->rate_ms > 0.0) {
        double factor = 2500.0 / st->rate_ms;
        if (factor > 1.02)      swprintf(chip, (size_t)chip_n, L"×%d", (int)(factor + 0.5));
        else if (factor < 0.98) swprintf(chip, (size_t)chip_n, L"÷%d", (int)(1.0 / factor + 0.5));
        else                    wcscpy(chip, L"×1");
        wcscpy(desc, L"Скорость изменения времени");
        *chip_color = p->accent;
        return;
    }
    if (st->mode == SRT_MODE_NATURAL) {
        wcscpy(chip, L"Обычное");
        wcscpy(desc, L"Время идёт с обычной скоростью игры");
        *chip_color = p->ok;
        return;
    }
    wcscpy(chip, L"Стоп");
    wcscpy(desc, L"Время остановлено");
    *chip_color = p->danger;
}

static void draw_button(UiState *st, HDC hdc, const UiButton *b, BOOL hot)
{
    const UiPalette *p = ui_theme_palette();
    COLORREF bg, fg, edge;
    switch (b->style) {
    case UI_BTN_ACCENT:
        bg = hot ? p->accent_hi : p->accent;
        fg = RGB(255, 255, 255);
        edge = bg;
        break;
    case UI_BTN_DANGER:
        bg = hot ? RGB(120, 52, 52) : RGB(96, 44, 44);
        fg = RGB(255, 225, 225);
        edge = bg;
        break;
    default:
        bg = hot ? p->btn_hi : p->btn;
        fg = p->btn_txt;
        edge = p->card_edge;
        break;
    }
    int r = px(st, 9);
    gfx_fill_round_rect(hdc, b->rect.left, b->rect.top,
                        b->rect.right - b->rect.left, b->rect.bottom - b->rect.top,
                        r, bg);
    if (b->style == UI_BTN_NORMAL)
        gfx_draw_round_rect(hdc, b->rect.left, b->rect.top,
                            b->rect.right - b->rect.left, b->rect.bottom - b->rect.top,
                            r, edge, 1);
    gfx_text(hdc, b->label, b->rect.left, b->rect.top,
             b->rect.right - b->rect.left, b->rect.bottom - b->rect.top,
             st->f_btn, fg, GFX_ALIGN_CENTER, GFX_LINE_CENTER);
}

void ui_paint(UiState *st, HDC hdc, int width, int height)
{
    UiMetrics m;
    metrics(st, width, height, &m);
    const UiPalette *p = ui_theme_palette();

    /* Двойная буферизация: сначала рисуем в offscreen-буфер и только потом
     * переносим его на экран. Без этого окно мерцает при каждой перерисовке. */
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = mem ? CreateCompatibleBitmap(hdc, width, height) : NULL;
    HBITMAP old = NULL;
    if (mem && bmp) old = (HBITMAP)SelectObject(mem, bmp);
    HDC target = (mem && bmp) ? mem : hdc;

    HBRUSH bg = CreateSolidBrush(p->bg);
    RECT all = { 0, 0, width, height };
    FillRect(target, &all, bg);
    DeleteObject(bg);

    /* заголовок */
    gfx_text(target, L"SnowRunner Time Jumper",
             m.x, m.title_y, m.w - m.theme_w - px(st, 12), px(st, 26),
             st->f_title, p->text, GFX_ALIGN_NEAR, GFX_LINE_CENTER);

    WCHAR sub[160];
    if (st->session) {
        const SrtInfo *info = srt_info(st->session);
        WCHAR proc[64], pid[32];
        gfx_widen(info ? info->process : SRT_GAME_PROCESS, proc, 64);
        /* PID в строку — вручную, чтобы не зависеть от swprintf. */
        unsigned long v = info ? (unsigned long)info->pid : 0UL;
        pid[0] = (WCHAR)('0' + (int)((v / 100000000UL) % 10));
        pid[1] = (WCHAR)('0' + (int)((v / 10000000UL) % 10));
        pid[2] = (WCHAR)('0' + (int)((v / 1000000UL) % 10));
        pid[3] = (WCHAR)('0' + (int)((v / 100000UL) % 10));
        pid[4] = (WCHAR)('0' + (int)((v / 10000UL) % 10));
        pid[5] = (WCHAR)('0' + (int)((v / 1000UL) % 10));
        pid[6] = (WCHAR)('0' + (int)((v / 100UL) % 10));
        pid[7] = (WCHAR)('0' + (int)((v / 10UL) % 10));
        pid[8] = (WCHAR)('0' + (int)(v % 10));
        pid[9] = 0;

        wcscpy(sub, proc);
        wcscat(sub, L" · PID ");
        wcscat(sub, pid);
        wcscat(sub, L" · память игры изменена");
    } else {
        wcscpy(sub, L"Соединение с игрой не установлено");
    }
    gfx_text(target, sub, m.x, m.sub_y, m.w - m.theme_w - px(st, 12), px(st, 18),
             st->f_sub, p->faint, GFX_ALIGN_NEAR, GFX_LINE_CENTER);

    /* карточка с часами */
    gfx_fill_round_rect(target, m.card_x, m.card_y, m.card_w, m.card_h, px(st, 16), p->card);
    gfx_draw_round_rect(target, m.card_x, m.card_y, m.card_w, m.card_h, px(st, 16), p->card_edge, 1);

    WCHAR clock_text[32];
    if (st->clock_valid)
        ui_format_clock(st->clock_hours, clock_text, 32);
    else
        wcscpy(clock_text, L"--:--");

    gfx_text(target, clock_text, m.clock_x, m.clock_y, m.clock_w, m.clock_h,
             st->f_clock, p->text, GFX_ALIGN_NEAR, GFX_LINE_CENTER);

    WCHAR chip[32], mdesc[128];
    COLORREF chip_color;
    mode_info(st, chip, 32, mdesc, 128, &chip_color);

    gfx_fill_round_rect(target, m.chip_x, m.chip_y, m.chip_w, m.chip_h, px(st, 15), chip_color);
    gfx_text(target, chip, m.chip_x, m.chip_y, m.chip_w, m.chip_h,
             st->f_chip, p->chip_txt, GFX_ALIGN_CENTER, GFX_LINE_CENTER);

    gfx_text(target, mdesc, m.mode_x, m.mode_y, m.mode_w, m.mode_h,
             st->f_sub, p->dim, GFX_ALIGN_NEAR, GFX_LINE_CENTER);

    /* заголовки разделов */
    gfx_text(target, L"УПРАВЛЕНИЕ ВРЕМЕНЕМ", m.x, m.sect_y, m.w, px(st, 18),
             st->f_sect, p->dim, GFX_ALIGN_NEAR, GFX_LINE_CENTER);

    /* кнопки */
    for (int i = 0; i < st->button_count; i++) {
        const UiButton *b = &st->buttons[i];
        draw_button(st, target, b, st->hover_id == b->id);
    }

    /* горячие клавиши */
    gfx_text(target, L"ГОРЯЧИЕ КЛАВИШИ", m.x, m.sect_y + px(st, 174), m.w, px(st, 18),
             st->f_sect, p->dim, GFX_ALIGN_NEAR, GFX_LINE_CENTER);

    int rows_per_col = 12;
    for (int i = 0; i < g_hotkey_count; i++) {
        const HotkeyDef *hk = &g_hotkeys[i];
        int col = i / rows_per_col;
        int row = i % rows_per_col;
        if (col > 1) break;
        int cx = m.x + col * (m.col_w + px(st, 12));
        int cy = m.list_y + row * m.list_row_h;

        WCHAR key[64], desc[96];
        gfx_widen(hk->keys, key, 64);
        gfx_widen(hk->desc, desc, 96);
        gfx_text(target, key, cx, cy, m.key_w, m.list_row_h,
                 st->f_key, p->accent, GFX_ALIGN_NEAR, GFX_LINE_CENTER);
        gfx_text(target, desc, cx + m.key_w + px(st, 6), cy,
                 m.col_w - m.key_w - px(st, 6), m.list_row_h,
                 st->f_desc, p->dim, GFX_ALIGN_NEAR, GFX_LINE_CENTER);
    }

    /* строка состояния */
    COLORREF sc = p->dim;
    if (st->status_kind == 1) sc = p->warn;
    if (st->status_kind == 2) sc = p->danger;
    gfx_text_wrapped(target, st->status, m.x, m.status_y, m.status_w, px(st, 34),
                     st->f_status, sc, GFX_ALIGN_NEAR, GFX_LINE_NEAR);

    /* вынос на экран */
    if (mem && bmp) {
        GdiFlush();
        BitBlt(hdc, 0, 0, width, height, mem, 0, 0, SRCCOPY);
        SelectObject(mem, old);
        DeleteObject(bmp);
        DeleteDC(mem);
    }
}
