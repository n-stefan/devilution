#ifndef EMSCRIPTEN
#include <windows.h>
#endif
#include "diablo.h"
#include "storm/storm.h"

BYTE *sgpBackBuf;
BYTE *sgpFrontBuf;
int sgdwLockCount;
BYTE *gpBuffer;
#ifdef _DEBUG
int locktbl[256];
#endif
char gbBackBuf;
char gbEmulate;
BOOL use_offscreen = FALSE;

void lock_buf_priv();
void unlock_buf_priv();
void dx_reinit();

#ifndef EMSCRIPTEN
HDC hDC;
HBITMAP hBitmap;
HDC hPaintDC;
HFONT hFont;
HRGN hClip = NULL;
#endif

void dx_init()
{
#ifndef EMSCRIPTEN
  hDC = GetDC(ghMainWnd);
  int x = GetDeviceCaps(hDC, LOGPIXELSY);
  hPaintDC = CreateCompatibleDC( hDC );
  hBitmap = CreateCompatibleBitmap( hDC, SCREEN_WIDTH, SCREEN_HEIGHT );
  SelectObject( hPaintDC, hBitmap );

  hFont = CreateFont( -17, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 0x12, "Times New Roman" );
  SelectObject( hPaintDC, hFont );

  SetFocus(ghMainWnd);
	ShowWindow(ghMainWnd, SW_SHOWNORMAL);
#endif

	palette_init();

  sgpBackBuf = (BYTE*) DiabloAllocPtr( BUFFER_HEIGHT * BUFFER_WIDTH );
  sgpFrontBuf = (BYTE*) DiabloAllocPtr( SCREEN_WIDTH * SCREEN_HEIGHT * 4 );
}

void lock_buf(BYTE idx)
{
#ifdef _DEBUG
	++locktbl[idx];
#endif
	lock_buf_priv();
}

void lock_buf_priv()
{
	gpBuffer = sgpBackBuf;
  sgdwLockCount++;
}

void unlock_buf(BYTE idx)
{
#ifdef _DEBUG
	if (!locktbl[idx])
		app_fatal("Draw lock underflow: 0x%x", idx);
	--locktbl[idx];
#endif
	unlock_buf_priv();
}

void unlock_buf_priv()
{
	if (sgdwLockCount == 0)
		app_fatal("draw main unlock error");
	if (gpBuffer == NULL)
		app_fatal("draw consistency error");

	sgdwLockCount--;
	if (sgdwLockCount == 0) {
		gpBufEnd -= (size_t)gpBuffer;
		gpBuffer = NULL;
	}
}

void dx_cleanup()
{
#ifndef EMSCRIPTEN
	if (ghMainWnd)
		ShowWindow(ghMainWnd, SW_HIDE);
	MemFreeDbg(sgpBackBuf);
  MemFreeDbg( sgpFrontBuf );
#endif
	sgdwLockCount = 0;
	gpBuffer = NULL;
}

void dx_reinit()
{
	ClearCursor();
}

#ifdef EMSCRIPTEN

#include <emscripten.h>

EM_JS( void, api_draw_begin, (), {
  self.DApi.draw_begin();
});
EM_JS( void, api_draw_blit, (int x, int y, int w, int h, void* ptr), {
  self.DApi.draw_blit(x, y, w, h, HEAPU8.subarray(ptr, ptr + w * h * 4));
});
EM_JS( void, api_draw_end, (), {
  self.DApi.draw_end();
});
EM_JS( void, api_draw_clip_text, (int x0, int y0, int x1, int y1), {
  self.DApi.draw_clip_text(x0, y0, x1, y1);
});
EM_JS( void, api_draw_text, (int x, int y, const char* ptr, int color), {
  var end = HEAPU8.indexOf(0, ptr);
  var text = String.fromCharCode.apply(null, HEAPU8.subarray(ptr, end));
  self.DApi.draw_text(x, y, text, color);
});
EM_JS(void, api_set_cursor, (DWORD x, DWORD y), {
  self.DApi.set_cursor(x, y);
});
//EM_JS(void, api_draw_belt, (int* items), {
//  self.DApi.draw_belt(HEAP32.subarray(items / 4, items / 4 + 8));
//});

BOOL draw_lock(int* ysize) {
  api_draw_begin();
  return TRUE;
}

void draw_unlock() {
  if (use_offscreen) {
    api_draw_blit(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, sgpFrontBuf);
  }
}

void draw_flush() {
  api_draw_end();
}

void draw_blit(DWORD dwX, DWORD dwY, DWORD dwWdt, DWORD dwHgt) {
  int nSrcOff, nDstOff = 0, nSrcWdt, nDstWdt = 0;

  /// ASSERT: assert(! (dwX & 3));
  /// ASSERT: assert(! (dwWdt & 3));

  nSrcOff = SCREENXY(dwX, dwY);
  nSrcWdt = BUFFER_WIDTH - dwWdt;
  if (use_offscreen) {
    nDstOff = dwX * (SCREEN_BPP / 8) + dwY * (SCREEN_WIDTH * SCREEN_BPP / 8);
    nDstWdt = (SCREEN_WIDTH * SCREEN_BPP / 8) - dwWdt * (SCREEN_BPP / 8);
  }

  lock_buf(6);

  /// ASSERT: assert(gpBuffer);

  int wdt, hgt;
  BYTE *src, *dst;

  src = &gpBuffer[nSrcOff];
  dst = (BYTE *) sgpFrontBuf + nDstOff;

  for (hgt = 0; hgt < dwHgt; hgt++, src += nSrcWdt, dst += nDstWdt) {
    for (wdt = 0; wdt < dwWdt; wdt++) {
      PALETTEENTRY pal = system_palette[*src++];
      dst[0] = pal.peRed;
      dst[1] = pal.peGreen;
      dst[2] = pal.peBlue;
      dst[3] = 255;
      dst += 4;
    }
  }

  unlock_buf(6);

  if (!use_offscreen) {
    api_draw_blit(dwX, dwY, dwWdt, dwHgt, sgpFrontBuf);
  }
}

void draw_clip_text(int x0, int y0, int x1, int y1) {
  if (x0 > 0 || y0 > 0 || x1 < SCREEN_WIDTH || y1 < SCREEN_HEIGHT) {
    api_draw_clip_text(x0, y0, x1, y1);
  }
}

#define RGB(r,g,b)          ((DWORD)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

void draw_text(int x, int y, const char *text, int color) {
  PALETTEENTRY pal = system_palette[color];
  api_draw_text(x, y, text, RGB(pal.peRed, pal.peGreen, pal.peBlue));
}

void _SetCursorPos(DWORD x, DWORD y) {
  api_set_cursor(x, y);
}

void draw_belt(int* items) {
  //api_draw_belt(items);
}

#else

BOOL draw_lock(int* ysize) {
  return TRUE;
}

void draw_unlock()
{
  BITMAPINFOHEADER bi;
  bi.biSize = sizeof bi;
  bi.biWidth = SCREEN_WIDTH;
  bi.biHeight = -SCREEN_HEIGHT;
  bi.biPlanes = 1;
  bi.biBitCount = 32;
  bi.biCompression = BI_RGB;
  bi.biSizeImage = 0;
  bi.biXPelsPerMeter = 1;
  bi.biYPelsPerMeter = 1;
  bi.biClrUsed = 0;
  bi.biClrImportant = 0;
  SetDIBits( hDC, hBitmap, 0, SCREEN_HEIGHT, sgpFrontBuf, (BITMAPINFO*) &bi, DIB_RGB_COLORS );
}

void draw_flush()
{
  BitBlt(hDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hPaintDC, 0, 0, SRCCOPY);
}


void draw_clip_text(int x0, int y0, int x1, int y1) {
  if (hClip) {
    SelectClipRgn(hPaintDC, NULL);
    DeleteObject(hClip);
    hClip = NULL;
  }
  if (x0 > 0 || y0 > 0 || x1 < SCREEN_WIDTH || y1 < SCREEN_HEIGHT) {
    hClip = CreateRectRgn(x0, y0, x1, y1);
    SelectClipRgn(hPaintDC, hClip);
  }
}

void draw_text(int x, int y, const char *text, int color) {
  PALETTEENTRY pal = system_palette[color];
  SetTextColor(hPaintDC, RGB(pal.peRed, pal.peGreen, pal.peBlue));
  SetBkMode(hPaintDC, TRANSPARENT);
  TextOut(hPaintDC, x, y, text, strlen(text));
}

void _SetCursorPos(DWORD x, DWORD y) {
  POINT pt;
  pt.x = x;
  pt.y = y;
  ClientToScreen(ghMainWnd, &pt);
  SetCursorPos(pt.x, pt.y);
}

void draw_blit(DWORD dwX, DWORD dwY, DWORD dwWdt, DWORD dwHgt) {
  int nSrcOff, nDstOff, nSrcWdt, nDstWdt;

  /// ASSERT: assert(! (dwX & 3));
  /// ASSERT: assert(! (dwWdt & 3));

  nSrcOff = SCREENXY(dwX, dwY);
  nDstOff = dwX * (SCREEN_BPP / 8) + dwY * (SCREEN_WIDTH * SCREEN_BPP / 8);
  nSrcWdt = BUFFER_WIDTH - dwWdt;
  nDstWdt = (SCREEN_WIDTH * SCREEN_BPP / 8) - dwWdt * (SCREEN_BPP / 8);
  dwWdt >>= 2;

  lock_buf(6);

  /// ASSERT: assert(gpBuffer);

  int wdt, hgt;
  BYTE *src, *dst;

  src = &gpBuffer[nSrcOff];
  dst = (BYTE *) sgpFrontBuf + nDstOff;

  for (hgt = 0; hgt < dwHgt; hgt++, src += nSrcWdt, dst += nDstWdt) {
    for (wdt = 0; wdt < 4 * dwWdt; wdt++) {
      PALETTEENTRY pal = system_palette[*src++];
      dst[0] = pal.peBlue;
      dst[1] = pal.peGreen;
      dst[2] = pal.peRed;
      dst += 4;
    }
  }

  unlock_buf(6);
}

void draw_belt(int* items) {
}

#endif
