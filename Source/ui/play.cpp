#include "common.h"
#include <vector>
#include "../storm/storm.h"

#include "../trace.h"

class MainState : public GameState {
public:
  MainState(BOOL bNewGame, BOOL bSinglePlayer) {
    newGame_ = bNewGame;
    singlePlayer_ = bSinglePlayer;
  }

  void onActivate() override {
    if (started_) {
      return;
    }
    byte_678640 = 1;
    BOOL fExitProgram = FALSE;
    if (!NetInit(singlePlayer_, &fExitProgram)) {
      gbRunGameResult = !fExitProgram;
      if (fExitProgram) {
        activate(nullptr);
      } else {
        activate(get_main_menu_dialog());
      }
      return;
    }
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

    //nthread_ignore_mutex(TRUE);
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
    //nthread_ignore_mutex(FALSE);
    started_ = true;
  }

  void stop() {
    if (started_) {
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
    _SNetDestroy();
  }

  void onRender(unsigned int time) override {
    for (int msg : messages) {
      if (gbMaxPlayers > 1) {
        pfile_write_hero();
      }
      //nthread_ignore_mutex(TRUE);
      PaletteFadeOut(8);
      FreeMonsterSnd();
      music_stop();
      track_repeat_walk(FALSE);
      sgbMouseDown = 0;
      //ReleaseCapture();
      ShowProgress(msg);
      drawpanflag = 255;
      DrawAndBlit();
      if (gbRunGame) {
        PaletteFadeIn(8);
      }
      //nthread_ignore_mutex(FALSE);
      gbGameLoopStartup = TRUE;
      drawpanflag = 255;
    }
    messages.clear();

    if (gbRunGame && time > lastLoop_ + 50) {
      diablo_color_cyc_logic();
      multi_process_network_packets();
      game_loop(gbGameLoopStartup);
      msgcmd_send_chat();
      gbGameLoopStartup = FALSE;
      DrawAndBlit();
      lastLoop_ = time;
    }

    if (!gbRunGame) {
      stop();
      if (!gbRunGameResult) {
        activate(nullptr);
      } else {
        activate(get_single_player_dialog());
      }
    }
  }

  void onMouse(const MouseEvent &e) override {
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

  void onKey(const KeyEvent &e) override {
    if (e.action == KeyEvent::Press) {
      PressKey(e.key);
    } else {
      ReleaseKey(e.key);
    }
  }

  void onChar(char chr) override {
    PressChar(chr);
  }

  std::vector<int> messages;

private:
  BOOL newGame_;
  BOOL singlePlayer_;
  unsigned int lastLoop_ = 0;
  bool started_ = false;
};

void post_event(int code) {
  if (auto ptr = dynamic_cast<MainState*>(GameState::current())) {
    ptr->messages.push_back(code);
  }
}

GameStatePtr get_play_state(const char* name, int mode) {
  strcpy(gszHero, name);
  gbLoadGame = (mode == SELHERO_CONTINUE);
  pfile_create_player_description(NULL, 0);
  return new MainState(TRUE, TRUE);
}
