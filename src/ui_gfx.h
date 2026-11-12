/*
 * ui_gfx.h — тонкий слой над GDI+ для отрисовки окна.
 *
 * Объявления функций GDI+ приведены здесь вручную (flat API), чтобы
 * один и тот же исходник собирался и компилятором MSVC, и MinGW-w64
 * без каких-либо дополнительных заголовков.
 */
#ifndef UI_GFX_H_INCLUDED
#define UI_GFX_H_INCLUDED

#include <windows.h>

/* --- типы GDI+ ---
 * GpGraphics объявлен как непрозрачная структура: в API GDI+ он передаётся
 * двойным указателем. Остальные объекты — обычные указатели на непрозрачные
 * структуры, поэтому в объявлениях ниже используется void*. */
typedef int   GpStatus;
typedef float REAL;
typedef DWORD ARGB;

typedef struct GpGraphics GpGraphics;

typedef void *GpBrush;
typedef void *GpSolidFill;
typedef void *GpPen;
typedef void *GpFont;
typedef void *GpFontFamily;
typedef void *GpStringFormat;
typedef void *GpFontCollection;

typedef struct { REAL X, Y; }               GpPointF;
typedef struct { REAL X, Y, Width, Height; } GpRectF;

typedef struct {
    UINT32 GdiplusVersion;
    void  *DebugEventCallback;
    BOOL   SuppressBackgroundThread;
    BOOL   SuppressExternalCodecs;
} GfxStartupInput;

/* --- значения перечислений --- */
#define GFX_OK                        0
#define GFX_SMOOTH_ANTIALIAS          4
#define GFX_TEXT_ANTIALIAS_GRIDFIT    3
#define GFX_UNIT_PIXEL                2
#define GFX_FONT_REGULAR              0
#define GFX_FONT_BOLD                 1
#define GFX_ALIGN_NEAR                0
#define GFX_ALIGN_CENTER              1
#define GFX_ALIGN_FAR                 2
#define GFX_LINE_NEAR                 0
#define GFX_LINE_CENTER               1
#define GFX_TRIM_ELLIPSIS             3
#define GFX_FORMAT_NO_WRAP            0x00001000

/* --- функции GDI+ --- */
GpStatus GdiplusStartup(ULONG_PTR *token, const GfxStartupInput *input, void *output);
void     GdiplusShutdown(ULONG_PTR token);
GpStatus GdipCreateFromHDC(HDC hdc, GpGraphics **graphics);
GpStatus GdipDeleteGraphics(GpGraphics *graphics);
GpStatus GdipSetSmoothingMode(GpGraphics *graphics, int mode);
GpStatus GdipSetTextRenderingHint(GpGraphics *graphics, int hint);
GpStatus GdipCreateSolidFill(ARGB color, GpSolidFill **brush);
GpStatus GdipDeleteBrush(GpBrush brush);
GpStatus GdipCreatePen1(ARGB color, REAL width, int unit, GpPen **pen);
GpStatus GdipDeletePen(GpPen pen);
GpStatus GdipFillRectangle(GpGraphics *g, GpBrush b, REAL x, REAL y, REAL w, REAL h);
GpStatus GdipFillEllipse(GpGraphics *g, GpBrush b, REAL x, REAL y, REAL w, REAL h);
GpStatus GdipFillPie(GpGraphics *g, GpBrush b, REAL x, REAL y, REAL w, REAL h,
                     REAL startAngle, REAL sweepAngle);
GpStatus GdipDrawArc(GpGraphics *g, GpPen p, REAL x, REAL y, REAL w, REAL h,
                     REAL startAngle, REAL sweepAngle);
GpStatus GdipDrawLine(GpGraphics *g, GpPen p, REAL x1, REAL y1, REAL x2, REAL y2);
GpStatus GdipDrawRectangle(GpGraphics *g, GpPen p, REAL x, REAL y, REAL w, REAL h);
GpStatus GdipDrawEllipse(GpGraphics *g, GpPen p, REAL x, REAL y, REAL w, REAL h);
GpStatus GdipCreateFontFamilyFromName(const WCHAR *name, GpFontCollection coll, GpFontFamily **family);
GpStatus GdipDeleteFontFamily(GpFontFamily family);
/* Внимание: порядок параметров — как у конструктора Gdiplus::Font:
 * (семейство, размер, стиль, единица измерения, результат). */
GpStatus GdipCreateFont(const GpFontFamily family, REAL emSize, int style, int unit, GpFont **font);
GpStatus GdipDeleteFont(GpFont font);
GpStatus GdipCreateStringFormat(int flags, LANGID lang, GpStringFormat **fmt);
GpStatus GdipDeleteStringFormat(GpStringFormat fmt);
GpStatus GdipSetStringFormatAlign(GpStringFormat fmt, int align);
GpStatus GdipSetStringFormatLineAlign(GpStringFormat fmt, int align);
GpStatus GdipSetStringFormatTrimming(GpStringFormat fmt, int trim);
GpStatus GdipDrawString(GpGraphics *g, const WCHAR *str, int len, const GpFont font,
                        const GpRectF *rect, const GpStringFormat fmt, GpBrush brush);

/* Сохранение картинки в файл (используется для диагностического снимка окна). */
GpStatus GdipCreateBitmapFromHBITMAP(HBITMAP hbm, void **bitmap);
GpStatus GdipSaveImageToFile(void *bitmap, const WCHAR *filename,
                              const GUID *clsidEncoder, UINT encoderParams, void *param);
GpStatus GdipDisposeImage(void *image);

/* ------------------------------------------------------------------ */
/*  Собственные примитивы                                               */
/* ------------------------------------------------------------------ */

typedef struct GfxFont GfxFont;

/* Инициализация/завершение GDI+. */
BOOL gfx_init(void);
void gfx_shutdown(void);

/* Шрифты (создаются один раз и переиспользуются). */
GfxFont *gfx_font_create(int pixel_size, int bold);
void    gfx_font_destroy(GfxFont *f);

/* Заливка и рамка скруглённого прямоугольника. */
void gfx_fill_round_rect(HDC hdc, int x, int y, int w, int h, int radius, COLORREF color);
void gfx_draw_round_rect(HDC hdc, int x, int y, int w, int h, int radius, COLORREF color, int width);

/* Заливка обычного прямоугольника. */
void gfx_fill_rect(HDC hdc, int x, int y, int w, int h, COLORREF color);

/* Текст. align: GFX_ALIGN_NEAR/CENTER/FAR, valign: GFX_LINE_NEAR/CENTER. */
void gfx_text(HDC hdc, const WCHAR *text, int x, int y, int w, int h,
              GfxFont *font, COLORREF color, int align, int valign);

/* Текст с переносом строк (для длинных сообщений). */
void gfx_text_wrapped(HDC hdc, const WCHAR *text, int x, int y, int w, int h,
                      GfxFont *font, COLORREF color, int align, int valign);

/* Простейший перевод строки в wide. */
void gfx_widen(const char *src, WCHAR *dst, int dst_chars);

#endif /* UI_GFX_H_INCLUDED */
