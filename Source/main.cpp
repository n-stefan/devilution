#ifdef EMSCRIPTEN
#include <emscripten.h>
#else
#include <windows.h>
#endif
#include "diablo.h"
#include "storm/storm.h"
#include "ui/diabloui.h"
#include "ui/common.h"

#include <vector>

DWORD TickCount = 0;
DWORD _GetTickCount() {
  return TickCount;
}

#ifdef EMSCRIPTEN

extern "C" {

EMSCRIPTEN_KEEPALIVE void DApi_Init(unsigned int time);
EMSCRIPTEN_KEEPALIVE void DApi_Mouse(int action, int button, int mods, int x, int y);
EMSCRIPTEN_KEEPALIVE void DApi_Key(int action, int mods, int key);
EMSCRIPTEN_KEEPALIVE void DApi_Char(int chr);
EMSCRIPTEN_KEEPALIVE void DApi_Render(unsigned int time);

}

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

EMSCRIPTEN_KEEPALIVE
void DApi_Init(unsigned int time) {
  TickCount = time;
  ghInst = NULL;
  srand(time);
  InitHash();
  diablo_init_screen();
  init_create_window(0);
  sound_init();
  UiInitialize(effects_play_sound);

  GameState::activate(initial_state());
}

EMSCRIPTEN_KEEPALIVE
void DApi_Mouse(int action, int button, int mods, int x, int y) {
  MouseEvent e;
  e.action = (MouseEvent::Action) action;
  e.button = button;
  e.modifiers = mods;
  e.x = x;
  e.y = y;
  GameState::processMouse(e);
}

EMSCRIPTEN_KEEPALIVE
void DApi_Key(int action, int mods, int key) {
  KeyEvent e;
  e.action = (KeyEvent::Action) action;
  e.modifiers = mods;
  e.key = key;
  GameState::processKey(e);
}

EMSCRIPTEN_KEEPALIVE
void DApi_Char(int chr) {
  GameState::processChar((char) chr);
}

EMSCRIPTEN_KEEPALIVE
void DApi_Render(unsigned int time) {
  TickCount = time;
  if (!GameState::current()) {
    GameState::activate(initial_state());
  }
  GameState::render(time);
}

#else

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
  HINSTANCE hInst;

  hInst = hInstance;
  ghInst = hInst;

  ShowCursor( FALSE );
  srand( GetTickCount() );
  InitHash();

  diablo_init_screen();
  diablo_parse_flags( lpCmdLine );
  init_create_window( nCmdShow );
  sound_init();
  UiInitialize( effects_play_sound );

  GameStatePtr nextState = get_title_dialog();

#ifndef SPAWN
  {
    int nData;
    char szValueName[] = "Intro";
    if ( !SRegLoadValue( "Diablo", szValueName, 0, &nData ) )
      nData = 1;
    if (nData) {
      nextState = get_video_state("gendata\\diablo1.smk", true, false, nextState);
    }
    SRegSaveValue( "Diablo", szValueName, 0, 0 );
  }
#endif

  //UiTitleDialog(7);
  //BlackPalette();

  nextState = get_video_state("gendata\\logo.smk", true, false, nextState);
  GameState::activate(nextState);
  nextState = nullptr;

  MSG msg;
  while (GameState::current()) {
    unsigned int time = GetTickCount();
    TickCount = time;
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
//    } else if (!nthread_has_500ms_passed(FALSE)) {
//#ifdef SLEEPFIX
//      Sleep(1);
//#endif
//      continue;
    }
    GameState::render(time);
  }

  //StartGame(TRUE, TRUE);
  //return;

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

#endif
