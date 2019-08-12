#ifndef EMSCRIPTEN
#include <windows.h>
#include <windowsx.h>
#else
#include <emscripten.h>
#endif
#include "diablo.h"
#include "storm/storm.h"
#include "ui/diabloui.h"
#include "ui/common.h"

_SNETVERSIONDATA fileinfo;
int gbActive;
HANDLE diabdat_mpq;

uint32_t gdwProductVersion = (1 << 16);

char gszVersionNumber[MAX_PATH] = "Version 1.0.0";
char gszProductName[MAX_PATH] = "DiabloWeb v1.0.0";

void set_client_version(int v0, int v1, int v2) {
  gdwProductVersion = (v0 << 16) | (v1 << 8) | v2;
  sprintf(gszVersionNumber, "Version %d.%d.%d", v0, v1, v2);
  sprintf(gszProductName, "DiabloWeb v%d.%d.%d", v0, v1, v2);
}

void api_current_save_id(int id) {
#ifdef EMSCRIPTEN
  EM_ASM_({
    self.DApi.current_save_id($0);
  }, id);
#endif
}
