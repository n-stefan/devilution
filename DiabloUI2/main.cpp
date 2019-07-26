#include "common.h"

BYTE* FontTables[4];
Art ArtFonts[4][2];
Art ArtLogos[3];
Art ArtFocus[3];
Art ArtBackground;
Art ArtCursor;
Art ArtHero;

void LoadUiGFX()
{
  FontTables[AFT_SMALL] = LoadFileInMem( "ui_art\\font16.bin", 0 );
  FontTables[AFT_MED] = LoadFileInMem( "ui_art\\font24.bin", 0 );
  FontTables[AFT_BIG] = LoadFileInMem( "ui_art\\font30.bin", 0 );
  FontTables[AFT_HUGE] = LoadFileInMem( "ui_art\\font42.bin", 0 );
  LoadArtFont( "ui_art\\font16s.pcx", AFT_SMALL, AFC_SILVER );
  LoadArtFont( "ui_art\\font16g.pcx", AFT_SMALL, AFC_GOLD );
  LoadArtFont( "ui_art\\font24s.pcx", AFT_MED, AFC_SILVER );
  LoadArtFont( "ui_art\\font24g.pcx", AFT_MED, AFC_GOLD );
  LoadArtFont( "ui_art\\font30s.pcx", AFT_BIG, AFC_SILVER );
  LoadArtFont( "ui_art\\font30g.pcx", AFT_BIG, AFC_GOLD );
  LoadArtFont( "ui_art\\font42g.pcx", AFT_HUGE, AFC_GOLD );

  LoadMaskedArtFont( "ui_art\\smlogo.pcx", &ArtLogos[LOGO_MED], 15 );
  LoadMaskedArtFont( "ui_art\\focus16.pcx", &ArtFocus[FOCUS_SMALL], 8 );
  LoadMaskedArtFont( "ui_art\\focus.pcx", &ArtFocus[FOCUS_MED], 8 );
  LoadMaskedArtFont( "ui_art\\focus42.pcx", &ArtFocus[FOCUS_BIG], 8 );
  LoadMaskedArtFont( "ui_art\\cursor.pcx", &ArtCursor, 1, 0 );
  LoadArt( "ui_art\\heros.pcx", &ArtHero, 4 );
}

void __stdcall UiDestroy();
BOOL __stdcall UiTitleDialog( int a1 );

void __stdcall UiInitialize()
{
  Connect_LoadGFXAndStuff();
  local_LoadArtCursor();
  bn_prof_100021C4();
}
