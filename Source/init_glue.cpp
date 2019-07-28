#include "diablo.h"
#include "storm/storm.h"

_SNETVERSIONDATA fileinfo;

char gszVersionNumber[MAX_PATH] = "Version 1.0.9.2";
char gszProductName[MAX_PATH] = "Diablo v1.09";

HANDLE diabdat_mpq;
HANDLE patch_rt_mpq = NULL;

void init_create_window(int nCmdShow) {
  pfile_init_save_directory();
  dx_init(0);
  BlackPalette();
  snd_init(0);
  init_archives();
}

HANDLE init_test_access(const char *mpq_path) {
  HANDLE archive;
  if (SFileOpenArchive(mpq_path, 1000, FS_PC, &archive))
    return archive;
  return NULL;
}

void init_archives() {
  HANDLE fh;
#ifndef SPAWN
  diabdat_mpq = init_test_access("diabdat.mpq");
#else
  diabdat_mpq = init_test_access("spawn.mpq");
#endif
  if (!WOpenFile("ui_art\\title.pcx", &fh, TRUE))
#ifndef SPAWN
    FileErrDlg("Main program archive: diabdat.mpq");
#else
    FileErrDlg("Main program archive: spawn.mpq");
#endif
  WCloseFile(fh);
}
