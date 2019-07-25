#include "diablo.h"
#include "../3rdParty/Storm/Source/storm.h"

BYTE *sgpBackBuf;
BYTE *sgpFrontBuf;
LPDIRECTDRAW lpDDInterface;
IDirectDrawPalette *lpDDPalette;
int sgdwLockCount;
BYTE *gpBuffer;
IDirectDrawSurface *lpDDSBackBuf;
IDirectDrawSurface *lpDDSPrimary;
#ifdef _DEBUG
int locktbl[256];
#endif
#ifdef __cplusplus
static CCritSect sgMemCrit;
#endif
char gbBackBuf;
char gbEmulate;
HMODULE ghDiabMod;

void lock_buf_priv();
void unlock_buf_priv();
void dx_reinit();
void dx_create_back_buffer();
void dx_create_primary_surface();
HRESULT dx_DirectDrawCreate( LPGUID guid, LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter );

HDC hDC;
HBITMAP hBitmap;
HDC hPaintDC;

void dx_init(HWND hWnd)
{
  ghMainWnd = hWnd;

  hDC = GetDC( hWnd );
  hPaintDC = CreateCompatibleDC( hDC );
  hBitmap = CreateCompatibleBitmap( hDC, SCREEN_WIDTH, SCREEN_HEIGHT );
  SelectObject( hPaintDC, hBitmap );

  SetFocus(hWnd);
	ShowWindow(hWnd, SW_SHOWNORMAL);
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
	if (ghMainWnd)
		ShowWindow(ghMainWnd, SW_HIDE);
	MemFreeDbg(sgpBackBuf);
  MemFreeDbg( sgpFrontBuf );
	sgdwLockCount = 0;
	gpBuffer = NULL;
}

void dx_reinit()
{
	ClearCursor();
}

BOOL draw_lock( int* ysize )
{
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
  BitBlt( hDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hPaintDC, 0, 0, SRCCOPY );
}

void draw_blit( DWORD dwX, DWORD dwY, DWORD dwWdt, DWORD dwHgt )
{
  int nSrcOff, nDstOff, nSrcWdt, nDstWdt;

  /// ASSERT: assert(! (dwX & 3));
  /// ASSERT: assert(! (dwWdt & 3));

  nSrcOff = SCREENXY( dwX, dwY );
  nDstOff = dwX * ( SCREEN_BPP / 8 ) + dwY * ( SCREEN_WIDTH * SCREEN_BPP / 8 );
  nSrcWdt = BUFFER_WIDTH - dwWdt;
  nDstWdt = ( SCREEN_WIDTH * SCREEN_BPP / 8 ) - dwWdt * ( SCREEN_BPP / 8 );
  dwWdt >>= 2;

  lock_buf( 6 );

  /// ASSERT: assert(gpBuffer);

  int wdt, hgt;
  BYTE *src, *dst;

  src = &gpBuffer[nSrcOff];
  dst = (BYTE *) sgpFrontBuf + nDstOff;

  for ( hgt = 0; hgt < dwHgt; hgt++, src += nSrcWdt, dst += nDstWdt )
  {
    for ( wdt = 0; wdt < 4 * dwWdt; wdt++ )
    {
      PALETTEENTRY pal = system_palette[*src++];
      dst[0] = pal.peBlue;
      dst[1] = pal.peGreen;
      dst[2] = pal.peRed;
      dst += 4;
    }
  }

  unlock_buf( 6 );
}
