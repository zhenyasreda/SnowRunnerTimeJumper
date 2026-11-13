/*
 * ui_gfx.c — реализация примитивов отрисовки (GDI+).
 */
#include <stdlib.h>
#include <string.h>
#include "ui_gfx.h"
#include "srt.h"
#include "i18n.h"

static ULONG_PTR g_token = 0;
static BOOL       g_started = FALSE;

/* COLORREF (0x00BBGGRR) -> ARGB (0xAARRGGBB). */
static ARGB to_argb(COLORREF c)
{
    return (ARGB)(0xFF000000u
                  | ((DWORD)(c & 0x0000FFu) << 16)   /* R */
                  |  (DWORD)(c & 0x00FF00u)           /* G */
                  | ((DWORD)(c & 0xFF0000u) >> 16));   /* B */
}

struct GfxFont {
    GpFont        *font;
    GpFontFamily  *family;
    GpStringFormat *format;
    GpStringFormat *format_wrap;
};

BOOL gfx_init(void)
{
    if (g_started) return TRUE;
    GfxStartupInput input;
    memset(&input, 0, sizeof(input));
    input.GdiplusVersion = 1;
    input.SuppressBackgroundThread = FALSE;
    input.SuppressExternalCodecs = FALSE;
    if (GdiplusStartup(&g_token, &input, NULL) != GFX_OK) return FALSE;
    g_started = TRUE;
    return TRUE;
}

void gfx_shutdown(void)
{
    if (!g_started) return;
    GdiplusShutdown(g_token);
    g_started = FALSE;
}

GfxFont *gfx_font_create(int pixel_size, int bold)
{
    if (!g_started) return NULL;
    GfxFont *f = (GfxFont *)calloc(1, sizeof(GfxFont));
    if (!f) return NULL;

    if (GdipCreateFontFamilyFromName(L"Segoe UI", NULL, &f->family) != GFX_OK) {
        log_msg(S_LOG_GFX_FAMILY);
        free(f);
        return NULL;
    }
    GpStatus s2 = GdipCreateFont(f->family, (REAL)pixel_size,
                                 bold ? GFX_FONT_BOLD : GFX_FONT_REGULAR,
                                 GFX_UNIT_PIXEL, &f->font);
    if (s2 != GFX_OK) {
        log_msg(S_LOG_GFX_FONT, pixel_size, bold ? 1 : 0, (int)s2);
        GdipDeleteFontFamily(f->family);
        free(f);
        return NULL;
    }
    if (GdipCreateStringFormat(GFX_FORMAT_NO_WRAP, LANG_NEUTRAL, &f->format) != GFX_OK) {
        GdipDeleteFont(f->font);
        GdipDeleteFontFamily(f->family);
        free(f);
        return NULL;
    }
    GdipSetStringFormatTrimming(f->format, GFX_TRIM_ELLIPSIS);
    GdipSetStringFormatLineAlign(f->format, GFX_LINE_CENTER);

    /* Второй формат — с переносом строк (для длинных сообщений). */
    if (GdipCreateStringFormat(0, LANG_NEUTRAL, &f->format_wrap) != GFX_OK)
        f->format_wrap = NULL;
    else
        GdipSetStringFormatTrimming(f->format_wrap, GFX_TRIM_ELLIPSIS);
    return f;
}

void gfx_font_destroy(GfxFont *f)
{
    if (!f) return;
    if (f->format_wrap) GdipDeleteStringFormat(f->format_wrap);
    if (f->format) GdipDeleteStringFormat(f->format);
    if (f->font)   GdipDeleteFont(f->font);
    if (f->family) GdipDeleteFontFamily(f->family);
    free(f);
}

/* Создать графический контекст и настроить его. */
static GpGraphics *begin(HDC hdc)
{
    GpGraphics *g = NULL;
    if (GdipCreateFromHDC(hdc, &g) != GFX_OK) return NULL;
    GdipSetSmoothingMode(g, GFX_SMOOTH_ANTIALIAS);
    GdipSetTextRenderingHint(g, GFX_TEXT_ANTIALIAS_GRIDFIT);
    return g;
}

void gfx_fill_rect(HDC hdc, int x, int y, int w, int h, COLORREF color)
{
    if (w <= 0 || h <= 0) return;
    GpGraphics *g = begin(hdc);
    if (!g) {
        RECT r = { x, y, x + w, y + h };
        HBRUSH b = CreateSolidBrush(color);
        FillRect(hdc, &r, b);
        DeleteObject(b);
        return;
    }
    GpSolidFill *brush = NULL;
    if (GdipCreateSolidFill(to_argb(color), &brush) == GFX_OK) {
        GdipFillRectangle(g, brush, (REAL)x, (REAL)y, (REAL)w, (REAL)h);
        GdipDeleteBrush(brush);
    }
    GdipDeleteGraphics(g);
}

/* Заливка скруглённого прямоугольника.
 * Углы — четверти круга (FillPie), середина — прямоугольники. Прямоугольники
 * намеренно выходят за углы на 1 пиксель: иначе на стыках остаются тонкие
 * «швы», а из-под углов торчат линии радиусов. */
void gfx_fill_round_rect(HDC hdc, int x, int y, int w, int h, int radius, COLORREF color)
{
    if (w <= 0 || h <= 0) return;
    int r = radius;
    int maxr = (w < h ? w : h) / 2;
    if (r > maxr) r = maxr;
    if (r <= 0) { gfx_fill_rect(hdc, x, y, w, h, color); return; }

    GpGraphics *g = begin(hdc);
    if (!g) {
        RECT rc = { x, y, x + w, y + h };
        HBRUSH b = CreateSolidBrush(color);
        FillRect(hdc, &rc, b);
        DeleteObject(b);
        return;
    }
    GpSolidFill *brush = NULL;
    if (GdipCreateSolidFill(to_argb(color), &brush) == GFX_OK) {
        REAL rr = (REAL)(2 * r);
        /* углы */
        GdipFillPie(g, brush, (REAL)x, (REAL)y, rr, rr, 180.0f, 90.0f);
        GdipFillPie(g, brush, (REAL)(x + w - 2 * r), (REAL)y, rr, rr, 270.0f, 90.0f);
        GdipFillPie(g, brush, (REAL)(x + w - 2 * r), (REAL)(y + h - 2 * r), rr, rr, 0.0f, 90.0f);
        GdipFillPie(g, brush, (REAL)x, (REAL)(y + h - 2 * r), rr, rr, 90.0f, 90.0f);
        /* середина (с перекрытием в 1 пиксель) */
        GdipFillRectangle(g, brush, (REAL)x, (REAL)(y + r - 1), (REAL)w, (REAL)(h - 2 * r + 2));
        GdipFillRectangle(g, brush, (REAL)(x + r - 1), (REAL)y, (REAL)(w - 2 * r + 2), (REAL)(r + 1));
        GdipFillRectangle(g, brush, (REAL)(x + r - 1), (REAL)(y + h - r - 1),
                          (REAL)(w - 2 * r + 2), (REAL)(r + 1));
        GdipDeleteBrush(brush);
    }
    GdipDeleteGraphics(g);
}

/* Рамка скруглённого прямоугольника: четыре прямые стороны и четыре дуги. */
void gfx_draw_round_rect(HDC hdc, int x, int y, int w, int h, int radius, COLORREF color, int width)
{
    if (w <= 0 || h <= 0) return;
    int r = radius;
    int maxr = (w < h ? w : h) / 2;
    if (r > maxr) r = maxr;
    if (r <= 0) {
        gfx_fill_rect(hdc, x, y, w, h, color);
        return;
    }

    GpGraphics *g = begin(hdc);
    if (!g) {
        HPEN p = CreatePen(PS_SOLID, width, color);
        HGDIOBJ old = SelectObject(hdc, p);
        HBRUSH b = (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH oldb = (HBRUSH)SelectObject(hdc, b);
        RoundRect(hdc, x, y, x + w, y + h, r * 2, r * 2);
        SelectObject(hdc, oldb);
        SelectObject(hdc, old);
        DeleteObject(p);
        return;
    }

    GpPen *pen = NULL;
    if (GdipCreatePen1(to_argb(color), (REAL)width, GFX_UNIT_PIXEL, &pen) == GFX_OK) {
        REAL rr = (REAL)(2 * r);
        REAL x1 = (REAL)x, y1 = (REAL)y, x2 = (REAL)(x + w), y2 = (REAL)(y + h);
        /* прямые стороны */
        GdipDrawLine(g, pen, (REAL)(x + r), y1, (REAL)(x + w - r), y1);
        GdipDrawLine(g, pen, (REAL)(x + r), y2, (REAL)(x + w - r), y2);
        GdipDrawLine(g, pen, x1, (REAL)(y + r), x1, (REAL)(y + h - r));
        GdipDrawLine(g, pen, x2, (REAL)(y + r), x2, (REAL)(y + h - r));
        /* угловые дуги */
        GdipDrawArc(g, pen, x1, y1, rr, rr, 180.0f, 90.0f);
        GdipDrawArc(g, pen, (REAL)(x + w - 2 * r), y1, rr, rr, 270.0f, 90.0f);
        GdipDrawArc(g, pen, (REAL)(x + w - 2 * r), (REAL)(y + h - 2 * r), rr, rr, 0.0f, 90.0f);
        GdipDrawArc(g, pen, x1, (REAL)(y + h - 2 * r), rr, rr, 90.0f, 90.0f);
        GdipDeletePen(pen);
    }
    GdipDeleteGraphics(g);
}

void gfx_text(HDC hdc, const WCHAR *text, int x, int y, int w, int h,
              GfxFont *font, COLORREF color, int align, int valign)
{
    gfx_text_wrapped(hdc, text, x, y, w, h, font, color, align, valign);
}

void gfx_text_wrapped(HDC hdc, const WCHAR *text, int x, int y, int w, int h,
                      GfxFont *font, COLORREF color, int align, int valign)
{
    if (!font || !text) return;
    GpGraphics *g = begin(hdc);
    if (!g) {
        RECT rc = { x, y, x + w, y + h };
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, color);
        DrawTextW(hdc, text, -1, &rc,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        return;
    }

    GpStringFormat *fmt = font->format_wrap ? font->format_wrap : font->format;
    GdipSetStringFormatAlign(fmt, align);
    GdipSetStringFormatLineAlign(fmt, valign);

    GpSolidFill *brush = NULL;
    if (GdipCreateSolidFill(to_argb(color), &brush) == GFX_OK) {
        GpRectF rect;
        rect.X = (REAL)x;
        rect.Y = (REAL)y;
        rect.Width = (REAL)w;
        rect.Height = (REAL)h;
        GdipDrawString(g, text, -1, font->font, &rect, fmt, brush);
        GdipDeleteBrush(brush);
    }
    GdipDeleteGraphics(g);
}

void gfx_widen(const char *src, WCHAR *dst, int dst_chars)
{
    if (!dst || dst_chars <= 0) return;
    int s = 0, d = 0;
    /* Индексы источника и приёмника считаются отдельно: в UTF-8 один
     * символ может занимать несколько байт, а в UTF-16 — один код. */
    while (src[s] && d + 1 < dst_chars) {
        unsigned char c = (unsigned char)src[s];
        unsigned int cp;
        int extra;
        if (c < 0x80) {
            cp = c; extra = 0;
        } else if ((c & 0xE0) == 0xC0) {
            cp = c & 0x1Fu; extra = 1;
        } else if ((c & 0xF0) == 0xE0) {
            cp = c & 0x0Fu; extra = 2;
        } else if ((c & 0xF8) == 0xF0) {
            cp = c & 0x07u; extra = 3;
        } else {
            cp = 0xFFFDu; extra = 0;
        }
        for (int k = 1; k <= extra; k++) {
            if (!src[s + k]) { extra = k - 1; break; }
            cp = (cp << 6) | ((unsigned char)src[s + k] & 0x3Fu);
        }
        dst[d++] = (WCHAR)cp;
        s += extra + 1;
    }
    dst[d] = 0;
}
