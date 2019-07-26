//HEADER_GOES_HERE
#ifndef __DX_H__
#define __DX_H__

extern BYTE *gpBuffer;
extern char gbBackBuf;
extern char gbEmulate;
extern HMODULE ghDiabMod;

void dx_init(HWND hWnd);
void lock_buf(BYTE idx);
void unlock_buf(BYTE idx);
void dx_cleanup();

BOOL draw_lock( int* ysize );
void draw_unlock();
void draw_flush();
void draw_blit(DWORD dwX, DWORD dwY, DWORD dwWdt, DWORD dwHgt);

void draw_text( int x, int y, const char* text, int color );

/* data */

#endif /* __DX_H__ */
