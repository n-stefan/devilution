#pragma once

#include "../diablo.h"
#include "../../types.h"
#include "event.h"

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

typedef struct Art
{
  BYTE *data;
  DWORD width;
  DWORD height;
  bool masked = false;
  BYTE mask;
} Art;

extern BYTE *FontTables[4];
extern Art ArtFonts[4][2];
extern Art ArtLogos[3];
extern Art ArtFocus[3];
extern Art ArtBackground;
extern Art ArtCursor;
extern Art ArtHero;

void LoadArt(char *pszFile, Art *art, int frames, PALETTEENTRY *pPalette = nullptr);
void DrawArt(int screenX, int screenY, Art *art, int nFrame = 0, int drawW = 0);
void LoadBackgroundArt(char *pszFile);
void LoadMaskedArtFont(char *pszFile, Art *art, int frames, int mask = 250);
void LoadArtFont(char *pszFile, int size, int color);
void UiFadeReset();
void UiFadeIn(unsigned int time);

void UiPlayMoveSound();
void UiPlaySelectSound();

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

extern void(__stdcall *gfnSoundFunction )( char *file );

class GameState {
public:
  virtual ~GameState() {};

  static void activate(GameState* state);
  static bool running();

  static void render(unsigned int time);
  static void processMouse(const MouseEvent& e);
  static void processKey(const KeyEvent& e);

protected:
  virtual void onRender(unsigned int time) = 0;
  virtual void onMouse(const MouseEvent& e) = 0;
  virtual void onKey(const KeyEvent& e) = 0;

private:
  void retain();
  void release();
  int counter_ = 0;
};

GameState* get_title_dialog();
GameState* get_main_menu_dialog();
