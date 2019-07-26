#include "diablo.h"
#include "../3rdParty/Storm/Source/storm.h"
#include "ui/diabloui.h"
#include "ui/common.h"

#include <vector>

class MainState : public GameState {
public:
  MainState(BOOL bNewGame, BOOL bSinglePlayer) {
    newGame_ = bNewGame;
    singlePlayer_ = bSinglePlayer;
  }

  bool start() {
    byte_678640 = 1;
    BOOL fExitProgram = FALSE;
    if (!NetInit(singlePlayer_, &fExitProgram)) {
      gbRunGameResult = !fExitProgram;
      return false;
    }
    started_ = true;
    byte_678640 = 0;
    if (newGame_ || !gbValidSaveFile) {
      InitLevels();
      InitQuests();
      InitPortals();
      InitDungMsgs(myplr);
    }
    unsigned int uMsg;
    if (!gbValidSaveFile || !gbLoadGame) {
      uMsg = WM_DIABNEWGAME;
    } else {
      uMsg = WM_DIABLOADGAME;
    }

    nthread_ignore_mutex(TRUE);
    start_game(uMsg);
    /// ASSERT: assert(ghMainWnd);
    control_update_life_mana();
    msg_process_net_packets();
    gbRunGame = TRUE;
    gbProcessPlayers = TRUE;
    gbRunGameResult = TRUE;
    drawpanflag = 255;
    DrawAndBlit();
    PaletteFadeIn(8);
    drawpanflag = 255;
    gbGameLoopStartup = TRUE;
    nthread_ignore_mutex(FALSE);
    return true;
  }

  void stop() {
    started_ = false;
    if (gbMaxPlayers > 1) {
      pfile_write_hero();
    }

    pfile_flush_W();
    PaletteFadeOut(8);
    SetCursor_(0);
    ClearScreenBuffer();
    drawpanflag = 255;
    scrollrt_draw_game_screen(TRUE);
    free_game();

    if (cineflag) {
      cineflag = FALSE;
      DoEnding();
    }

    NetClose();
    pfile_create_player_description(0, 0);
  }

  ~MainState() {
    if (started_) {
      stop();
    }
    SNetDestroy();
  }

  void onRender( unsigned int time ) override {
    if (!started_) {
      if (!start()) {
        mainmenu_refresh_music();
        GameState::activate(get_main_menu_dialog());
        return;
      }
    }

    for (int msg : messages) {
      if (gbMaxPlayers > 1) {
        pfile_write_hero();
      }
      nthread_ignore_mutex(TRUE);
      PaletteFadeOut(8);
      FreeMonsterSnd();
      music_stop();
      track_repeat_walk(FALSE);
      sgbMouseDown = 0;
      ReleaseCapture();
      ShowProgress(msg);
      drawpanflag = 255;
      DrawAndBlit();
      if (gbRunGame) {
        PaletteFadeIn(8);
      }
      nthread_ignore_mutex(FALSE);
      gbGameLoopStartup = TRUE;
    }
    messages.clear();

    if (gbRunGame) {
		  diablo_color_cyc_logic();
      multi_process_network_packets();
      game_loop(gbGameLoopStartup);
      msgcmd_send_chat();
      gbGameLoopStartup = FALSE;
      DrawAndBlit();
    }

    if (!gbRunGame) {
      stop();
      if (!gbRunGameResult) {
        mainmenu_refresh_music();
        GameState::activate(get_main_menu_dialog());
      }
    }
  }

  void onMouse( const MouseEvent &e ) override {
    MouseX = e.x;
    MouseY = e.y;
    if (e.action == MouseEvent::Press) {
      if (e.button == KeyCode::LBUTTON) {
        if (sgbMouseDown == 0) {
          sgbMouseDown = 1;
          track_repeat_walk(LeftMouseDown(e.modifiers & ModifierKey::SHIFT));
        }
      } else if (e.button == KeyCode::RBUTTON) {
        if (sgbMouseDown == 0) {
          sgbMouseDown = 2;
          RightMouseDown();
        }
      }
    } else if (e.action == MouseEvent::Release) {
      if (e.button == KeyCode::LBUTTON) {
        if (sgbMouseDown == 1) {
          sgbMouseDown = 0;
          LeftMouseUp();
          track_repeat_walk(FALSE);
        }
      } else if (e.button == KeyCode::RBUTTON) {
        if (sgbMouseDown == 2) {
          sgbMouseDown = 0;
        }
      }
    } else {
      gmenu_on_mouse_move();
    }
  }

  void onKey( const KeyEvent &e ) override {
    if (e.action == KeyEvent::Press) {
      PressKey(e.key);
    } else {
      ReleaseKey(e.key);
    }
  }

  void onChar( char chr ) override {
    PressChar(chr);
  }

  std::vector<int> messages;

private:
  BOOL newGame_;
  BOOL singlePlayer_;
  bool started_ = false;
};

void post_event( int code ) {
  if ( auto ptr = dynamic_cast<MainState*>( GameState::current() ) ) {
    ptr->messages.push_back(code);
  }
}

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
  HINSTANCE hInst;
  int nData;

  hInst = hInstance;
  ghInst = hInst;

  ShowCursor( FALSE );
  srand( GetTickCount() );
  InitHash();
  fault_get_filter();

  diablo_init_screen();
  diablo_parse_flags( lpCmdLine );
  init_create_window( nCmdShow );
  sound_init();
  UiInitialize( effects_play_sound );

  GameState *nextState = get_title_dialog();

  {
    char szValueName[] = "Intro";
    if ( !SRegLoadValue( "Diablo", szValueName, 0, &nData ) )
      nData = 1;
    if (nData) {
      nextState = get_video_state("gendata\\diablo1.smk", true, false, nextState);
    }
    SRegSaveValue( "Diablo", szValueName, 0, 0 );
  }

  //UiTitleDialog(7);
  //BlackPalette();

  nextState = get_video_state("gendata\\logo.smk", true, false, nextState);

  GameState::activate(nextState);

  MSG msg;
  while (GameState::current()) {
    unsigned int time = GetTickCount();
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
      bool bLoop = gbRunGame && nthread_has_500ms_passed(FALSE);
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
      if (!bLoop) {
        continue;
      }
    } else if (!nthread_has_500ms_passed(FALSE)) {
#ifdef SLEEPFIX
      Sleep(1);
#endif
      continue;
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
