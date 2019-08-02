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

void api_current_save_id(int id);

#endif /* __INIT_H__ */
