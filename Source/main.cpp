#ifndef EMSCRIPTEN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include "rmpq/archive.h"
#endif
#include "diablo.h"
#include "storm/storm.h"
#include "ui/diabloui.h"
#include "ui/common.h"

//#define INSTANT_LOAD

DWORD TickCount = 0;
DWORD _GetTickCount() {
  return TickCount;
}

#ifdef INSTANT_LOAD
#include <vector>

static std::vector<_uiheroinfo> _heroes;
static BOOL  SelHero_GetHeroInfo( _uiheroinfo *pInfo )
{
  _heroes.push_back( *pInfo );
  return TRUE;
}
#endif

#ifdef EMSCRIPTEN
#include <emscripten.h>

extern "C" {

EMSCRIPTEN_KEEPALIVE void DApi_Init(unsigned int time, int offscreen, int v0, int v1, int v2);
EMSCRIPTEN_KEEPALIVE void DApi_Mouse(int action, int button, int mods, int x, int y);
EMSCRIPTEN_KEEPALIVE void DApi_Key(int action, int mods, int key);
EMSCRIPTEN_KEEPALIVE void DApi_Char(int chr);
EMSCRIPTEN_KEEPALIVE void DApi_Render(unsigned int time);

}

EM_JS(void, api_exit_game, (), {
  self.DApi.exit_game();
});

#else

void api_exit_game() {
}

#endif

GameStatePtr initial_state() {
  GameStatePtr nextState = get_title_dialog();

#ifndef SPAWN
  {
    int nData;
    char szValueName[] = "Intro";
    if (!SRegLoadValue("Diablo", szValueName, 0, &nData))
      nData = 1;
    if (nData) {
      nextState = get_video_state("gendata\\diablo1.smk", true, false, nextState);
    }
    SRegSaveValue("Diablo", szValueName, 0, 0);
  }
#endif

  return get_video_state("gendata\\logo.smk", true, false, nextState);
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

void DApi_Init(unsigned int time, int offscreen, int v0, int v1, int v2) {
  sprintf(gszVersionNumber, "Version %d.%d.%d", v0, v1, v2);
  sprintf(gszProductName, "DiabloWeb v%d.%d.%d", v0, v1, v2);

  TickCount = time;
  use_offscreen = offscreen;
  srand(time);
  InitHash();
  diablo_init_screen();
  pfile_init_save_directory();
  dx_init();
  BlackPalette();
  snd_init();
  init_archives();
  sound_init();
  UiInitialize(effects_play_sound);
  gbActive = 1;

#ifndef INSTANT_LOAD
  GameState::activate(initial_state());
#else
  gbMaxPlayers = 1;
  pfile_ui_set_hero_infos( SelHero_GetHeroInfo );
  if ( _heroes.empty() )
  {
    _uidefaultstats defaults;
    _uiheroinfo hero;
    pfile_ui_set_class_stats( UI_WARRIOR, &defaults );
    hero.level = 1;
    hero.heroclass = UI_WARRIOR;
    hero.strength = defaults.strength;
    hero.magic = defaults.magic;
    hero.dexterity = defaults.dexterity;
    hero.vitality = defaults.vitality;
    strcpy( hero.name, "Riv" );
    pfile_ui_save_create( &hero );
    GameState::activate( get_play_state( hero.name, SELHERO_NEW_DUNGEON ) );
  }
  else
  {
    GameState::activate( get_play_state( _heroes[0].name, SELHERO_CONTINUE ) );
  }
#endif
}

void DApi_Mouse(int action, int button, int mods, int x, int y) {
  MouseEvent e;
  e.action = (MouseEvent::Action) action;
  e.button = button;
  e.modifiers = mods;
  e.x = x;
  e.y = y;
  GameState::processMouse(e);
}

void DApi_Key(int action, int mods, int key) {
  KeyEvent e;
  e.action = (KeyEvent::Action) action;
  e.modifiers = mods;
  e.key = key;
  GameState::processKey(e);
}

void DApi_Char(int chr) {
  GameState::processChar((char) chr);
}

void DApi_Render(unsigned int time) {
  TickCount = time;
  if (!GameState::current()) {
    api_exit_game();
  }
  GameState::render(time);
}

#ifndef EMSCRIPTEN

LRESULT __stdcall MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

#include <algorithm>

bool validate_mpq()
{
  char path[MAX_PATH];
  int save_num = pfile_get_save_num_from_name( gszHero );
  pfile_get_save_path( path, sizeof( path ), save_num );

  File file( path );
  mpq::MPQHeader header;
  file.read( &header, mpq::MPQHeader::size_v1 );
  std::vector<mpq::MPQBlockEntry> blocks( header.blockTableSize );
  file.seek( header.blockTablePos, SEEK_SET );
  file.read( blocks.data(), blocks.size() * sizeof( mpq::MPQBlockEntry ) );
  mpq::decryptBlock( blocks.data(), blocks.size() * sizeof( mpq::MPQBlockEntry ), mpq::hashString( "(block table)", mpq::HASH_KEY ) );

  std::vector<mpq::MPQBlockEntry> usedBlocks;
  for ( auto& block : blocks )
  {
    if ( block.cSize || block.filePos )
    {
      usedBlocks.push_back( block );
    }
  }
  std::sort( usedBlocks.begin(), usedBlocks.end(), []( const mpq::MPQBlockEntry& a, const mpq::MPQBlockEntry& b )
  {
    return a.filePos < b.filePos;
  } );
  uint32_t offset = 65640;
  for ( auto& block : usedBlocks )
  {
    if ( offset != block.filePos )
    {
      return false;
    }
    offset = block.filePos + block.cSize;
  }
  if ( offset != header.archiveSize )
  {
    return false;
  }
  return true;
}

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
  ghInst = hInstance;

  ShowCursor(FALSE);

  WNDCLASSEXA wcex;
  memset(&wcex, 0, sizeof(wcex));
  wcex.cbSize = sizeof(wcex);
  wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wcex.lpfnWndProc = MainWndProc;
  wcex.hInstance = ghInst;
  wcex.hIcon = LoadIcon(ghInst, MAKEINTRESOURCE(IDI_ICON1));
  wcex.hCursor = LoadCursor(0, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
  wcex.lpszMenuName = "DIABLO";
  wcex.lpszClassName = "DIABLO";
  wcex.hIconSm = (HICON) LoadImage(ghInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  if (!RegisterClassEx(&wcex))
    app_fatal("Unable to register window class");

  RECT rc = {0, 0, 500, 240};
  DWORD wsStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  DWORD wsExStyle = WS_EX_CLIENTEDGE;
  AdjustWindowRectEx(&rc, wsStyle, FALSE, wsExStyle);
  ghMainWnd = CreateWindowEx(wsExStyle, "DIABLO", "DIABLO", wsStyle, 0, 1300, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, ghInst, NULL);
  if (!ghMainWnd)
    app_fatal("Unable to create main window");
  ShowWindow(ghMainWnd, SW_SHOWNORMAL);
  UpdateWindow(ghMainWnd);

  DApi_Init(GetTickCount(), 0, 1, 0, 0);

  MSG msg;
  while (GameState::current()) {
    if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
          gbRunGameResult = FALSE;
          gbRunGame = FALSE;
          GameState::activate(nullptr);
          break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      bool bLoop = gbRunGame;// && nthread_has_500ms_passed(FALSE);
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
      if (!bLoop) {
        continue;
      }
    }
    DApi_Render(GetTickCount());
    Sleep(50);
  }

	music_stop();
  UiDestroy();
  SaveGamma();
  if ( ghMainWnd )
  {
    Sleep( 300 );
    DestroyWindow( ghMainWnd );
  }

  return FALSE;
}


LRESULT __stdcall MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
  switch (Msg) {
  case WM_ERASEBKGND:
    return 0;
  case WM_CREATE:
    ghMainWnd = hWnd;
    break;
  case WM_DESTROY:
    ghMainWnd = 0;
    PostQuitMessage(0);
    break;
  case WM_PAINT:
    drawpanflag = 255;
    break;
  case WM_CLOSE:
    return 0;
  case WM_MOUSEMOVE:
  case WM_LBUTTONDOWN:
  case WM_LBUTTONUP:
  case WM_RBUTTONDOWN:
  case WM_RBUTTONUP:
    if (GameState::current()) {
      int action = MouseEvent::Move;
      if (Msg == WM_LBUTTONDOWN || Msg == WM_RBUTTONDOWN) {
        action = MouseEvent::Press;
      } else if (Msg == WM_LBUTTONUP || Msg == WM_RBUTTONUP) {
        action = MouseEvent::Release;
      }
      int button = 0;
      if (Msg == WM_LBUTTONDOWN || Msg == WM_LBUTTONUP) {
        button = KeyCode::LBUTTON;
      } else if (Msg == WM_RBUTTONDOWN || Msg == WM_RBUTTONUP) {
        button = KeyCode::RBUTTON;
      }
      int mods = 0;
      if (wParam & MK_CONTROL) {
        mods |= ModifierKey::CONTROL;
      }
      if (wParam & MK_SHIFT) {
        mods |= ModifierKey::SHIFT;
      }
      DApi_Mouse(action, button, mods, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    }
    return 0;
  case WM_KEYDOWN:
  case WM_KEYUP:
    if (GameState::current()) {
      int action = (Msg == WM_KEYDOWN ? KeyEvent::Press : KeyEvent::Release);
      int mods = 0;
      if (GetKeyState(VK_CONTROL)) {
        mods |= ModifierKey::CONTROL;
      }
      if (GetKeyState(VK_SHIFT)) {
        mods |= ModifierKey::SHIFT;
      }
      DApi_Key(action, mods, (int) wParam);
    }
    return 0;
  case WM_CHAR:
    if (wParam >= 32 && wParam <= 128 && isprint(wParam)) {
      DApi_Char((int) wParam);
    }
    return 0;
  }

  return DefWindowProc(hWnd, Msg, wParam, lParam);
}

#endif
