#pragma once

#include "diabloui.h"
#include "../types.h"

typedef enum _artFocus
{
  FOCUS_SMALL,
  FOCUS_MED,
  FOCUS_BIG,
} _artFocus;

typedef enum _artLogo
{
  LOGO_SMALL,
  LOGO_MED,
  LOGO_BIG,
} _artLogo;

typedef enum _artFontTables
{
  AFT_SMALL,
  AFT_MED,
  AFT_BIG,
  AFT_HUGE,
} _artFontTables;

typedef enum _artFontColors
{
  AFC_SILVER,
  AFC_GOLD,
} _artFontColors;

typedef enum UiTypes
{
  UI_TEXT,
  UI_IMAGE,
  UI_BUTTON,
  UI_LIST,
  UI_EDIT,
} UiTypes;

typedef enum UiFlags
{
  UIS_SMALL = 1 << 0,
  UIS_MED = 1 << 1,
  UIS_BIG = 1 << 2,
  UIS_HUGE = 1 << 3,
  UIS_CENTER = 1 << 4,
  UIS_RIGHT = 1 << 5,
  UIS_VCENTER = 1 << 6,
  UIS_SILVER = 1 << 7,
  UIS_GOLD = 1 << 8,
  UIS_SML1 = 1 << 9,
  UIS_SML2 = 1 << 10,
  UIS_LIST = 1 << 11,
  UIS_DISABLED = 1 << 12,
  UIS_HIDDEN = 1 << 13,
} UiFlags;

typedef struct Art
{
  BYTE *data;
  DWORD width;
  DWORD height;
  bool masked = false;
  BYTE mask;
} Art;

typedef struct UI_Item
{
  RECT rect;
  UiTypes type;
  int flags;
  int value;
  char *caption;
  const void *context;
} UI_Item;

extern BYTE *FontTables[4];
extern Art ArtFonts[4][2];
extern Art ArtLogos[3];
extern Art ArtFocus[3];
extern Art ArtBackground;
extern Art ArtCursor;
extern Art ArtHero;

typedef enum TXT_JUST
{
  JustLeft = 0,
  JustCentre = 1,
  JustRight = 2,
} TXT_JUST;

template <class T, size_t N>
constexpr size_t size( T( &)[N] )
{
  return N;
}

extern void( *gfnSoundFunction )( char *file );
