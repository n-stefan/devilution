#include "common.h"

int SelectedItemMin = 1;
int SelectedItemMax = 1;
BYTE *FontTables[4];
Art ArtFonts[4][2];
Art ArtLogos[3];
Art ArtFocus[3];
Art ArtBackground;
Art ArtCursor;
Art ArtHero;

void( *gfnSoundFunction )( char *file );
void( *gfnListEsc )( );
bool( *gfnListYesNo )( );
int gUiItemCnt;
bool UiItemsWraps;
char *UiTextInput;
int UiTextInputLen;

int fadeValue = 0;
int SelectedItem = 0;

void UiDestroy()
{
  mem_free_dbg( ArtHero.data );
  ArtHero.data = NULL;
}

void UiPlayMoveSound()
{
  if ( gfnSoundFunction )
    gfnSoundFunction( "sfx\\items\\titlemov.wav" );
}

void UiPlaySelectSound()
{
  if ( gfnSoundFunction )
    gfnSoundFunction( "sfx\\items\\titlslct.wav" );
}

void UiFocus( int itemIndex, bool wrap = false )
{
  if ( !wrap )
  {
    if ( itemIndex < SelectedItemMin )
    {
      itemIndex = SelectedItemMin;
      return;
    }
    else if ( itemIndex > SelectedItemMax )
    {
      itemIndex = SelectedItemMax ? SelectedItemMax : SelectedItemMin;
      return;
    }
  }
  else if ( itemIndex < SelectedItemMin )
  {
    itemIndex = SelectedItemMax ? SelectedItemMax : SelectedItemMin;
  }
  else if ( itemIndex > SelectedItemMax )
  {
    itemIndex = SelectedItemMin;
  }

  if ( SelectedItem == itemIndex )
    return;

  SelectedItem = itemIndex;

  UiPlayMoveSound();

  if ( gfnListFocus )
    gfnListFocus( itemIndex );
}

void selhero_CatToName( char *in_buf, char *out_buf, int cnt )
{
  std::string output = utf8_to_latin1( in_buf );
  output.resize( SDL_TEXTINPUTEVENT_TEXT_SIZE - 1 );
  strncat( out_buf, output.c_str(), cnt - strlen( out_buf ) );
}

bool UiFocusNavigation( SDL_Event *event )
{
  if ( event->type == SDL_QUIT )
    exit( 0 );

  if ( event->type == SDL_KEYDOWN )
  {
    switch ( event->key.keysym.sym )
    {
    case SDLK_UP:
      UiFocus( SelectedItem - 1, UiItemsWraps );
      return true;
    case SDLK_DOWN:
      UiFocus( SelectedItem + 1, UiItemsWraps );
      return true;
    case SDLK_TAB:
      if ( SDL_GetModState() & KMOD_SHIFT )
        UiFocus( SelectedItem - 1, UiItemsWraps );
      else
        UiFocus( SelectedItem + 1, UiItemsWraps );
      return true;
    case SDLK_PAGEUP:
      UiFocus( SelectedItemMin );
      return true;
    case SDLK_PAGEDOWN:
      UiFocus( SelectedItemMax );
      return true;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
    case SDLK_SPACE:
      UiFocusNavigationSelect();
      return true;
    case SDLK_DELETE:
      UiFocusNavigationYesNo();
      return true;
    }
  }

  if ( SDL_IsTextInputActive() )
  {
    switch ( event->type )
    {
    case SDL_KEYDOWN:
      switch ( event->key.keysym.sym )
      {
      case SDLK_v:
        if ( SDL_GetModState() & KMOD_CTRL )
        {
          char *clipboard = SDL_GetClipboardText();
          if ( clipboard == NULL )
          {
            SDL_Log( SDL_GetError() );
          }
          selhero_CatToName( clipboard, UiTextInput, UiTextInputLen );
        }
        return true;
      case SDLK_BACKSPACE:
      case SDLK_LEFT:
        int nameLen = strlen( UiTextInput );
        if ( nameLen > 0 )
        {
          UiTextInput[nameLen - 1] = '\0';
        }
        return true;
      }
      break;
    case SDL_TEXTINPUT:
      selhero_CatToName( event->text.text, UiTextInput, UiTextInputLen );
      return true;
    }
  }

  if ( gUiItems && gUiItemCnt && UiItemMouseEvents( event, gUiItems, gUiItemCnt ) )
    return true;

  if ( gfnListEsc && event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_ESCAPE )
  {
    UiFocusNavigationEsc();
    return true;
  }

  return false;
}

void UiFocusNavigationSelect()
{
  UiPlaySelectSound();
  if ( SDL_IsTextInputActive() )
  {
    if ( strlen( UiTextInput ) == 0 )
    {
      return;
    }
    SDL_StopTextInput();
    UiTextInput = NULL;
    UiTextInputLen = 0;
  }
  if ( gfnListSelect )
    gfnListSelect( SelectedItem );
}

void UiFocusNavigationEsc()
{
  UiPlaySelectSound();
  if ( SDL_IsTextInputActive() )
  {
    SDL_StopTextInput();
    UiTextInput = NULL;
    UiTextInputLen = 0;
  }
  if ( gfnListEsc )
    gfnListEsc();
}

void UiFocusNavigationYesNo()
{
  if ( gfnListYesNo == NULL )
    return;

  if ( gfnListYesNo() )
    UiPlaySelectSound();
}

bool IsInsideRect( const SDL_Event *event, const SDL_Rect *rect )
{
  const SDL_Point point = { event->button.x, event->button.y };
  return SDL_PointInRect( &point, rect );
}

void LoadArt( char *pszFile, Art *art, int frames, PALETTEENTRY *pPalette )
{
  if ( art == NULL || art->data != NULL )
    return;

  if ( !SBmpLoadImage( pszFile, 0, 0, 0, &art->width, &art->height, 0 ) )
    return;

  art->data = (BYTE *) malloc( art->width * art->height );
  if ( !SBmpLoadImage( pszFile, pPalette, art->data, art->width * art->height, 0, 0, 0 ) )
    return;

  if ( art->data == NULL )
    return;

  art->height /= frames;
}

void LoadMaskedArtFont( char *pszFile, Art *art, int frames, int mask )
{
  LoadArt( pszFile, art, frames );
  art->masked = true;
  art->mask = mask;
}

void LoadArtFont( char *pszFile, int size, int color )
{
  LoadMaskedArtFont( pszFile, &ArtFonts[size][color], 256, 32 );
}

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

void InitFont()
{
  if ( !TTF_WasInit() && TTF_Init() == -1 )
  {
    printf( "TTF_Init: %s\n", TTF_GetError() );
    exit( 1 );
  }
  atexit( TTF_Quit );

  font = TTF_OpenFont( "CharisSILB.ttf", 17 );
  if ( font == NULL )
  {
    printf( "TTF_OpenFont: %s\n", TTF_GetError() );
    return;
  }

  TTF_SetFontKerning( font, false );
  TTF_SetFontHinting( font, TTF_HINTING_MONO );
}

void UiInitialize()
{
  LoadUiGFX();
  ShowCursor( false );
  InitFont();
}

int UiProfileGetString()
{
  DUMMY();
  return 0;
}

char connect_plrinfostr[128];
char connect_categorystr[128];
void UiSetupPlayerInfo( char *infostr, _uiheroinfo *pInfo, DWORD type )
{
  DUMMY();
  SStrCopy( connect_plrinfostr, infostr, 128 );
  char format[32] = "";
  strncpy( format, (char *) &type, 4 );
  strcat( format, " %d %d %d %d %d %d %d %d %d" );

  snprintf(
    connect_categorystr,
    128,
    format,
    pInfo->level,
    pInfo->heroclass,
    pInfo->herorank,
    pInfo->strength,
    pInfo->magic,
    pInfo->dexterity,
    pInfo->vitality,
    pInfo->gold,
    pInfo->spawned );
}

BOOL UiCopyProtError( int *pdwResult )
{
  UNIMPLEMENTED();
}

void UiAppActivate( BOOL bActive )
{
  DUMMY();
}

BOOL UiValidPlayerName( char *name )
{
  if ( !strlen( name ) )
    return false;

  if ( strpbrk( name, ",<>%&\\\"?*#/:" ) || strpbrk( name, " " ) )
    return false;

  for ( char *letter = name; *letter; letter++ )
    if ( *letter < 0x20 || *letter > 0x7E && *letter < 0xC0 )
      return false;

  return true;
}

void UiProfileCallback()
{
  UNIMPLEMENTED();
}

void UiProfileDraw()
{
  UNIMPLEMENTED();
}

BOOL UiCategoryCallback( int a1, int a2, int a3, int a4, int a5, DWORD *a6, DWORD *a7 )
{
  UNIMPLEMENTED();
}

BOOL UiGetDataCallback( int game_type, int data_code, void *a3, int a4, int a5 )
{
  UNIMPLEMENTED();
}

BOOL UiAuthCallback( int a1, char *a2, char *a3, char a4, char *a5, LPSTR lpBuffer, int cchBufferMax )
{
  UNIMPLEMENTED();
}

BOOL UiSoundCallback( int a1, int type, int a3 )
{
  UNIMPLEMENTED();
}

void UiMessageBoxCallback( HWND hWnd, char *lpText, LPCSTR lpCaption, UINT uType )
{
  UNIMPLEMENTED();
}

BOOL UiDrawDescCallback( int game_type, COLORREF color, LPCSTR lpString, char *a4, int a5, UINT align, time_t a7,
                         HDC *a8 )
{
  UNIMPLEMENTED();
}

BOOL UiCreateGameCallback( int a1, int a2, int a3, int a4, int a5, int a6 )
{
  UNIMPLEMENTED();
}

BOOL UiArtCallback( int game_type, unsigned int art_code, PALETTEENTRY *pPalette, BYTE *pBuffer,
                    DWORD dwBuffersize, DWORD *pdwWidth, DWORD *pdwHeight, DWORD *pdwBpp )
{
  UNIMPLEMENTED();
}

BOOL UiCreatePlayerDescription( _uiheroinfo *info, DWORD mode, char *desc )
{
  char format[32] = "";
  strncpy( format, (char *) &mode, 4 );
  strcat( format, " %d %d %d %d %d %d %d %d %d" );

  snprintf(
    desc,
    128,
    format,
    info->level,
    info->heroclass,
    info->herorank,
    info->strength,
    info->magic,
    info->dexterity,
    info->vitality,
    info->gold,
    info->spawned );

  return true;
}

void DrawArt( int screenX, int screenY, Art *art, int nFrame, int drawW )
{
  BYTE *src = (BYTE *) art->data + ( art->width * art->height * nFrame );
  BYTE *dst = &gpBuffer[screenX + 64 + ( screenY + SCREEN_Y ) * BUFFER_WIDTH];
  drawW = drawW ? drawW : art->width;

  for ( int i = 0; i < art->height && i + screenY < SCREEN_HEIGHT; i++, src += art->width, dst += BUFFER_WIDTH )
  {
    for ( int j = 0; j < art->width && j + screenX < SCREEN_WIDTH; j++ )
    {
      if ( j < drawW && ( !art->masked || src[j] != art->mask ) )
        dst[j] = src[j];
    }
  }
}

int GetCenterOffset( int w, int bw )
{
  if ( bw == 0 )
  {
    bw = SCREEN_WIDTH;
  }

  return bw / 2 - w / 2;
}

int GetStrWidth( char *str, int size )
{
  int strWidth = 0;

  for ( size_t i = 0; i < strlen( (char *) str ); i++ )
  {
    BYTE w = FontTables[size][*(BYTE *) &str[i] + 2];
    if ( w )
      strWidth += w;
    else
      strWidth += FontTables[size][0];
  }

  return strWidth;
}

int TextAlignment( UI_Item *item, TXT_JUST align, _artFontTables size )
{
  if ( align != JustLeft )
  {
    int w = GetStrWidth( item->caption, size );
    if ( align == JustCentre )
    {
      return GetCenterOffset( w, item->rect.w );
    }
    else if ( align == JustRight )
    {
      return item->rect.w - w;
    }
  }

  return 0;
}

void WordWrap( UI_Item *item )
{
  int lineStart = 0;
  int len = strlen( (char *) item->caption );
  for ( int i = 0; i <= len; i++ )
  {
    if ( item->caption[i] == '\n' )
    {
      lineStart = i + 1;
      continue;
    }
    else if ( item->caption[i] != ' ' && i != len )
    {
      continue;
    }

    if ( i != len )
      item->caption[i] = '\0';
    if ( GetStrWidth( &item->caption[lineStart], AFT_SMALL ) <= item->rect.w )
    {
      if ( i != len )
        item->caption[i] = ' ';
      continue;
    }

    int j;
    for ( j = i; j >= lineStart; j-- )
    {
      if ( item->caption[j] == ' ' )
      {
        break; // Scan for previous space
      }
    }

    if ( j == lineStart )
    { // Single word longer then width
      if ( i == len )
        break;
      j = i;
    }

    if ( i != len )
      item->caption[i] = ' ';
    item->caption[j] = '\n';
    lineStart = j + 1;
  }
};

void DrawArtStr( UI_Item *item )
{
  _artFontTables size = AFT_SMALL;
  _artFontColors color = item->flags & UIS_GOLD ? AFC_GOLD : AFC_SILVER;
  TXT_JUST align = JustLeft;

  if ( item->flags & UIS_MED )
    size = AFT_MED;
  else if ( item->flags & UIS_BIG )
    size = AFT_BIG;
  else if ( item->flags & UIS_HUGE )
    size = AFT_HUGE;

  if ( item->flags & UIS_CENTER )
    align = JustCentre;
  else if ( item->flags & UIS_RIGHT )
    align = JustRight;

  int x = item->rect.x + TextAlignment( item, align, size );

  int sx = x;
  int sy = item->rect.y;
  if ( item->flags & UIS_VCENTER )
    sy += ( item->rect.h - ArtFonts[size][color].height ) / 2;

  for ( size_t i = 0; i < strlen( (char *) item->caption ); i++ )
  {
    if ( item->caption[i] == '\n' )
    {
      sx = x;
      sy += ArtFonts[size][color].height;
      continue;
    }
    BYTE w = FontTables[size][*(BYTE *) &item->caption[i] + 2] ? FontTables[size][*(BYTE *) &item->caption[i] + 2] : FontTables[size][0];
    DrawArt( sx, sy, &ArtFonts[size][color], *(BYTE *) &item->caption[i], w );
    sx += w;
  }

  if ( item->type == UI_EDIT && GetAnimationFrame( 2, 500 ) )
  {
    DrawArt( sx, sy, &ArtFonts[size][color], '|' );
  }
}

void LoadPalInMem( PALETTEENTRY *pPal )
{
  for ( int i = 0; i < 256; i++ )
  {
    orig_palette[i].peFlags = 0;
    orig_palette[i].peRed = pPal[i].peRed;
    orig_palette[i].peGreen = pPal[i].peGreen;
    orig_palette[i].peBlue = pPal[i].peBlue;
  }
}

void LoadBackgroundArt( char *pszFile )
{
  PALETTEENTRY pPal[256];

  fadeValue = 0;
  LoadArt( pszFile, &ArtBackground, 1, pPal );
  LoadPalInMem( pPal );
  ApplyGamma( logical_palette, orig_palette, 256 );
}

int GetAnimationFrame( int frames, int fps )
{
  int frame = ( SDL_GetTicks() / fps ) % frames;

  return frame > frames ? 0 : frame;
}

void UiFadeIn( int steps )
{
  if ( fadeValue < 256 )
  {
    fadeValue += steps;
    if ( fadeValue > 256 )
    {
      fadeValue = 256;
    }
  }

  SetFadeLevel( fadeValue );
}

void UiRenderItemDebug( UI_Item item )
{
  item.rect.x += 64; // pal_surface is shifted?
  item.rect.y += SCREEN_Y;
  if ( SDL_FillRect( pal_surface, &item.rect, random( 0, 255 ) ) <= -1 )
  {
    SDL_Log( SDL_GetError() );
  }
}

void DrawSelector( UI_Item *item = 0 )
{
  int size = FOCUS_SMALL;
  if ( item->rect.h >= 42 )
    size = FOCUS_BIG;
  else if ( item->rect.h >= 30 )
    size = FOCUS_MED;

  int frame = GetAnimationFrame( 8 );
  int y = item->rect.y + ( item->rect.h - ArtFocus[size].height ) / 2; // TODO FOCUS_MED appares higher then the box

  DrawArt( item->rect.x, y, &ArtFocus[size], frame );
  DrawArt( item->rect.x + item->rect.w - ArtFocus[size].width, y, &ArtFocus[size], frame );
}

void DrawEditBox( UI_Item item )
{
  DrawSelector( &item );
  item.rect.x += 43;
  item.rect.y += 1;
  item.rect.w -= 86;
  DrawArtStr( &item );
}

void UiRender()
{
  SDL_Event event;
  while ( SDL_PollEvent( &event ) )
  {
    UiFocusNavigation( &event );
  }
  UiRenderItems( gUiItems, gUiItemCnt );
  DrawLogo();
  DrawMouse();
  UiFadeIn();
}

void UiRenderItems( UI_Item *items, int size )
{
  for ( int i = 0; i < size; i++ )
  {
    if ( items[i].flags & UIS_HIDDEN )
      continue;

    switch ( items[i].type )
    {
    case UI_EDIT:
      DrawEditBox( items[i] );
      break;
    case UI_LIST:
      if ( items[i].caption == NULL )
        continue;
      if ( SelectedItem == items[i].value )
        DrawSelector( &items[i] );
    case UI_BUTTON:
    case UI_TEXT:
      DrawArtStr( &items[i] );
      break;
    case UI_IMAGE:
      DrawArt( items[i].rect.x, items[i].rect.y, (Art *) items[i].context, items[i].value, items[i].rect.w );
      break;
    default:
      UiRenderItemDebug( items[i] );
      break;
    }
  }
}

bool UiItemMouseEvents( SDL_Event *event, UI_Item *items, int size )
{
  if ( event->type != SDL_MOUSEBUTTONDOWN || event->button.button != SDL_BUTTON_LEFT )
  {
    return false;
  }

  for ( int i = 0; i < size; i++ )
  {
    if ( !IsInsideRect( event, &items[i].rect ) )
    {
      continue;
    }

    if ( items[i].type != UI_BUTTON && items[i].type != UI_LIST )
    {
      continue;
    }

    if ( items[i].type == UI_LIST )
    {
      if ( items[i].caption != NULL && *items[i].caption != '\0' )
      {
        if ( gfnListFocus != NULL && SelectedItem != items[i].value )
        {
          UiFocus( items[i].value );
        }
        else if ( gfnListFocus == NULL || event->button.clicks >= 2 )
        {
          SelectedItem = items[i].value;
          UiFocusNavigationSelect();
        }
      }

      return true;
    }

    if ( items[i].context )
    {
      ( ( void( *)( int value ) )items[i].context )( items[i].value );
    }
    return true;
  }

  return false;
}

void DrawLogo( int t, int size )
{
  DrawArt( GetCenterOffset( ArtLogos[size].width ), t, &ArtLogos[size], GetAnimationFrame( 15 ) );
}

void DrawMouse()
{
  SDL_GetMouseState( &MouseX, &MouseY );

  if ( renderer )
  {
    float scaleX;
    SDL_RenderGetScale( renderer, &scaleX, NULL );
    MouseX /= scaleX;
    MouseY /= scaleX;

    SDL_Rect view;
    SDL_RenderGetViewport( renderer, &view );
    MouseX -= view.x;
    MouseY -= view.y;
  }

  DrawArt( MouseX, MouseY, &ArtCursor );
}

/**
 * @brief Get int from ini, if not found the provided value will be added to the ini instead
 */
void DvlIntSetting( const char *valuename, int *value )
{
  if ( !SRegLoadValue( "devilutionx", valuename, 0, value ) )
  {
    SRegSaveValue( "devilutionx", valuename, 0, *value );
  }
}

/**
 * @brief Get string from ini, if not found the provided value will be added to the ini instead
 */
void DvlStringSetting( const char *valuename, char *string, int len )
{
  if ( !SRegLoadString( "devilutionx", valuename, 0, string, len ) )
  {
    SRegSaveString( "devilutionx", valuename, 0, string );
  }
}
}
