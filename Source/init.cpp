#ifndef EMSCRIPTEN
#include <windows.h>
#include <windowsx.h>
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
