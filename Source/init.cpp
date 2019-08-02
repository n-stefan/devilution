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

/* data */

char gszVersionNumber[MAX_PATH] = "Version 1.0.9.2";
char gszProductName[MAX_PATH] = "Diablo v1.09";


void api_current_save_id(int id) {
#ifdef EMSCRIPTEN
  EM_ASM_({
    self.DApi.current_save_id($0);
  }, id);
#endif
}
