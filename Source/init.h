//HEADER_GOES_HERE
#ifndef __INIT_H__
#define __INIT_H__

extern _SNETVERSIONDATA fileinfo;
extern int gbActive;
extern HANDLE diabdat_mpq;

/* rdata */

/* data */

extern char gszVersionNumber[MAX_PATH];
extern char gszProductName[MAX_PATH];
extern uint32_t gdwProductVersion;

void set_client_version(int v0, int v1, int v2);

void api_current_save_id(int id);

void api_open_keyboard(int x0, int y0, int x1, int y1, int len);
void api_close_keyboard();

#endif /* __INIT_H__ */
